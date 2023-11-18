#include "uart.hpp"
#include <memory>
#include <errno.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uart_hw);

constexpr size_t UART_WAIT_FOR_BUF_DELAY = 50;
constexpr size_t UART_RX_TIMEOUT = 50;

/**
 * @brief FIFO buffer for UART data.
*/
K_FIFO_DEFINE(uart_rx_fifo);

/**
 * @brief Pipe used by the UART/NUS.
 * 
 * @details This pipe is used to pass data from the UART to the NUS.
*/
extern struct k_pipe nus_uart_pipe;

/**
 * @brief Make the UART instance available to the callback/handler.
*/
static Uart* uart_instance;

/**
 * @brief UART callback.
 * 
 * @param dev The UART device.
 * @param evt The UART event.
 * @param user_data User data.
*/
void Uart::uart_callback(const struct device *dev, struct uart_event *evt, void *user_data) {
  ARG_UNUSED(user_data);

  static size_t aborted_len;
  struct uart_data_t *buf;
  static uint8_t *aborted_buf;
  static bool disable_req;

  switch (evt->type) {
    case UART_TX_DONE: {
      // A UART TX finished. Fetch new or aborted data for next transmission.
      LOG_DBG("UART_TX_DONE");
      if ((evt->data.tx.len == 0) ||
          (!evt->data.tx.buf)) {
        // No data to send.
        return;
      }

      // get the uart data structure so we can free the memory so buf can be reused.
      if (aborted_buf) {
        buf = CONTAINER_OF(aborted_buf, uart_data_t, data);
        aborted_buf = NULL;
        aborted_len = 0;
      } else {
        buf = CONTAINER_OF(evt->data.tx.buf, uart_data_t, data);
      }

      // buf is freed to be used in the next transmission.
      // \todo: use delete instead of k_free
      k_free(buf);

      // Allocate more memory to get the next buffer from the pipe.
      buf = static_cast<uart_data_t*>(k_malloc(sizeof(*buf))); 
      if (!buf) {
        LOG_WRN("Not able to allocate UART receive buffer on done");
        return;
      }

      // Put the data into the pipe.
      buf->len = 0;
      size_t bytes_read;
      int err = k_pipe_get(&nus_uart_pipe, buf->data, UART_BUF_SIZE, &bytes_read, 0, K_NO_WAIT);
      if (err < 0) {
        LOG_WRN("nus get from nus_uart_pipe failed: %d", err);
        k_free(buf);
        return;
      }

      // Update the length based on actual bytes read
      buf->len = bytes_read;

      // Send the data over UART.
      if (uart_tx(dev, buf->data, buf->len, SYS_FOREVER_MS)) {
        LOG_WRN("Failed to send data over UART");
        // Don't free buf here. UART is async: uart_tx returns immediately and event handler, set using 
        // uart_callback_set (so this uart_callback f'n), is called after transfer is finished.
      }

      break;
    }
    case UART_RX_RDY: {
      // The UART is ready to receive data.
      LOG_DBG("UART_RX_RDY");

      // Get the buffer from the event.
      buf = CONTAINER_OF(evt->data.rx.buf, uart_data_t, data);
      buf->len += evt->data.rx.len;

      if (disable_req) {
        // RX is disabled so stop.
        return;
      }

      // Check for end of line. Disable RX if end of line is found.
      if ((evt->data.rx.buf[buf->len - 1] == '\n') ||
          (evt->data.rx.buf[buf->len - 1] == '\r')) {
        disable_req = true;
        uart_rx_disable(dev);
      }

      break;
    }
    case UART_RX_DISABLED: {
      // The UART RX is disabled.
      LOG_DBG("UART_RX_DISABLED");
      disable_req = false;

      // Allocate a new buffer using new.
      // \todo: Use make_unique for exception safety
      buf = static_cast<uart_data_t*>(k_malloc(sizeof(*buf)));
      if (buf) {
        buf->len = 0;
      } else {
        LOG_WRN("Not able to allocate UART receive buffer on disabled");
        // Reschedule the work to try again.
        k_work_reschedule(&uart_instance->uart_work_, K_MSEC(UART_WAIT_FOR_BUF_DELAY));
        return;
      }

      // Enable RX with the new buffer.
      // \todo: why can't we use uart_instance->dev_ here? It's null. uart_instance->uart_work_ seems fine.
      uart_rx_enable(dev, buf->data, sizeof(buf->data), UART_RX_TIMEOUT);

      break;
    }
    case UART_RX_BUF_REQUEST: {
      // A UART RX buffer is requested. Allocate a buffer and send it to the UART.
      LOG_DBG("UART_RX_BUF_REQUEST");

      // Allocate a new buffer and send it to the UART.
      buf = static_cast<uart_data_t*>(k_malloc(sizeof(*buf)));
      if (buf) {
        buf->len = 0;
        uart_rx_buf_rsp(dev, buf->data, sizeof(buf->data));
      } else {
        LOG_WRN("Not able to allocate UART receive buffer on request");
      }

      break;
    }
    case UART_RX_BUF_RELEASED: {
      // A UART buffer is released. Put buffer into FIFO (if not empty) or delete it.
      LOG_DBG("UART_RX_BUF_RELEASED");
      buf = CONTAINER_OF(evt->data.rx_buf.buf, uart_data_t, data);

      if (buf->len > 0) {
        k_fifo_put(&uart_rx_fifo, buf);
      } else {
        // \todo: use delete instead of k_free
        k_free(buf);
      }

      break;
    }
    case UART_TX_ABORTED: {
      // A UART TX was aborted. Save the buffer and length.
      LOG_DBG("UART_TX_ABORTED");
      if (!aborted_buf) {
        aborted_buf = const_cast<uint8_t *>(evt->data.tx.buf);
      }

      aborted_len += evt->data.tx.len;
      buf = CONTAINER_OF(aborted_buf, uart_data_t, data);

      // Send the rest of the aborted data over UART.
      uart_tx(dev, &buf->data[aborted_len], buf->len - aborted_len, SYS_FOREVER_MS);
      break;
    }
    default: {
      break;
    }
  }
}

/**
 * @brief UART delayable work handler.
 * 
 * @details This handler is used with the delayable work structure.
 *          It allocates a new buffer and enables UART RX.
 * 
 * @param item The work item.
*/
void Uart::uart_work_handler(struct k_work *item) {
	struct uart_data_t *buf;

  // \todo: use unique_ptr to ensure it's deallocated properly when it goes out of...
  // ...scope or an exception is thrown, preventing memory leaks.
  buf = static_cast<uart_data_t*>(k_malloc(sizeof(*buf)));
	if (buf) {
		buf->len = 0;
	} else {
		LOG_WRN("Not able to allocate UART receive buffer in handler");
		k_work_reschedule(&uart_instance->uart_work_, K_MSEC(UART_WAIT_FOR_BUF_DELAY));
		return;
	}

  // Enable RX. Pass it the raw pointer to the data buffer.
	uart_rx_enable(uart_instance->dev_, buf->data, sizeof(buf->data), UART_RX_TIMEOUT);
}

/**
 * @brief Initializes the UART.
 * 
 * @return int 0 if successful, otherwise negative error code.
*/
int Uart::init() {
  // Make the UART instance available to the static methods.
  uart_instance = this;

  int err;
  struct uart_data_t *rx;

  // Check if the UART device is ready.
  if (!device_is_ready(this->dev_)) {
    LOG_ERR("UART device not ready");
    return -ENODEV;
  }

  // Allocate a new buffer for the UART RX.
  // \todo: convert to std::make_unique: Using malloc because other C bits are using malloc/free.
  rx = static_cast<uart_data_t*>(k_malloc(sizeof(*rx)));
  if (rx) {
    rx->len = 0;
  } else {
    return -ENOMEM;
  }

  // Initialize the delayable work structure.
  k_work_init_delayable(&this->uart_work_, uart_work_handler);

  // Set the UART callback.
  err = uart_callback_set(this->dev_, uart_callback, nullptr);
  if (err) {
    k_free(rx);
    return err;
  }

  // Enable UART RX. Pass it the raw pointer to the data buffer.
  err = uart_rx_enable(this->dev_, rx->data, sizeof(rx->data), UART_RX_TIMEOUT);  
  if (err) {
    LOG_ERR("Cannot enable uart reception (err: %d)", err);
    k_free(rx);
  }

  LOG_INF("UART initialized");

  return err;
}