#include "thread_base.hpp"
#include "uart.hpp"
#include <errno.h>
#include <zephyr/kernel.h>
#include <bluetooth/services/nus.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>

#define NUS_WRITE_TIMEOUT K_MSEC(150)

LOG_MODULE_REGISTER(nus_thread);

/**
 * @brief Pipe used by the UART/NUS.
 * 
 * @details This pipe is used to pass data from the UART to the NUS.
*/
extern struct k_pipe uart_nus_pipe;

/**
 * @brief Semaphore used to signal that NUS can be initialized (BLE init is done).
*/
extern struct k_sem ble_init_done;

/**
 * @brief Pipes used by the UART and NUS (from BLE callback).
 * 
 * @details The naming format is <putter>_<getter>_pipe.
*/
K_PIPE_DEFINE(nus_uart_pipe, 1024, 4);

/**
 * @brief Thread for handling NUS (Nordic UART Sevice).
 * 
 * @details This thread is responsible for initializing the NUS and handling
 * 			    incoming data from the NUS. It passes the data to the nus_uart_pipe.
*/
class NusThread : public ThreadBase<NusThread> {
  friend class ThreadBase<NusThread>;

protected:
  bool init() override {
    // Don't go any further until BLE is initialized
	  k_sem_take(&ble_init_done, K_FOREVER);

    static struct bt_nus_cb nus_cb = {
      .received = bt_receive_cb,
    };

    // Initialize the NUS service.
    int err = bt_nus_init(&nus_cb);
    if (err) {
      LOG_ERR("Failed to initialize NUS service (err: %d)", err);
      return false;
    }

    LOG_INF("NUS module initialized");
    k_sem_give(&ble_init_done);
    return true;
  }

  void run() override {
    LOG_INF("[nus thread] starting");

    // get data from pipe
    size_t bytes_read;
    int err = k_pipe_get(&uart_nus_pipe, buf->data, UART_BUF_SIZE, &bytes_read, 0, K_FOREVER);
    buf->len = bytes_read;
    if (err < 0) {
      LOG_WRN("nus get from uart_nus_pipe failed: %d", err);
      return;
    }

    LOG_INF("[nus thread] buf->data: %s", buf->data);

    // Send the data over BLE.
    err = bt_nus_send(nullptr, buf->data, buf->len);
    if (err) {
      LOG_WRN("Failed to send data over BLE connection (err %d)", err);
    }

    LOG_INF("[nus thread] done");
  }

public:
  NusThread() {
    // Allocate memory for the buffer. Must use heap memory because the buffer is passed to NUS service.
    buf = static_cast<uart_data_t*>(k_malloc(sizeof(*buf)));
    if (!buf) {
      LOG_ERR("Not able to allocate UART receive buffer");
      return;
    }
  }

  ~NusThread() {
    k_free(buf);
  }

private:
  // Buffer used to store data received on NUS pipe and sent to BLE.
  struct uart_data_t *buf;

  /**
   * @brief Callback for when data is received from BLE.
   * 
   * @details This function is called when data is received from BLE. Data from BLE is
   *          passed to the pipe (i.e. to uart).
   * 
   * @param conn The connection object.
   * @param data The data received.
   * @param len The length of the data received.
  */
  static void bt_receive_cb(struct bt_conn *conn, const uint8_t *const data, uint16_t len) {
    char addr[BT_ADDR_LE_STR_LEN] = {0};
	  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, ARRAY_SIZE(addr));
	  LOG_INF("Received data from: %s", addr);

    for (uint16_t pos = 0; pos != len;) {
      // \todo: can stack be used here?
      struct uart_data_t *tx = static_cast<struct uart_data_t*>(k_malloc(sizeof(*tx)));
      if (!tx) {
        LOG_WRN("Not able to allocate UART send data buffer");
        return;
      }

      /* Keep the last byte of TX buffer for potential LF char. */
      int tx_data_size = sizeof(tx->data) - 1;

      if ((len - pos) > tx_data_size) {
        tx->len = tx_data_size;
      } else {
        tx->len = (len - pos);
      }

      // Copy the data received from ble to the buffer.
      memcpy(tx->data, &data[pos], tx->len);

      pos += tx->len;

      // Append the LF character when the CR character triggered transmission from the peer.
      if ((pos == len) && (data[len - 1] == '\r')) {
        tx->data[tx->len] = '\n';
        tx->len++;
      }

      // Send ble data to pipe
      size_t bytes_written;
      int rc = k_pipe_put(&nus_uart_pipe, tx->data, tx->len, &bytes_written, 0, K_NO_WAIT);
      tx->len = bytes_written;
      if (rc < 0) {
        LOG_WRN("nus put to nus_uart_pipe failed: %d", rc);
      }

      k_free(tx);
    }
  }
};

void nus_thread_function(void *arg0, void *arg1, void *arg2) {
  ARG_UNUSED(arg0);
  ARG_UNUSED(arg1);
  ARG_UNUSED(arg2);

  NusThread thread;
  thread();
}

K_THREAD_DEFINE(nus_thread_id, CONFIG_MAIN_STACK_SIZE, nus_thread_function, NULL, NULL, NULL, 4, 0, 0);