#include "thread_base.hpp"
#include "uart.hpp"
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uart_thread_log);

/**
 * @brief This pipe is used to pass data from the UART to the NUS.
 * 
 * @details The naming format is <putter>_<getter>_pipe.
*/
K_PIPE_DEFINE(uart_nus_pipe, 1024, 4);

/**
 * @brief FIFO buffer for UART data.
*/
extern struct k_fifo uart_rx_fifo;

/**
 * @brief Thread for handling UART.
 * 
 * @details This thread is responsible for initializing the UART and handling
 *          incoming data from the UART. It passes the data to the uart_nus_pipe.
*/
class UartThread : public ThreadBase<UartThread> {
  friend class ThreadBase<UartThread>;

protected:
  bool init() override {
    Uart uart;
    int err = uart.init();
    if (err != 0) {
      LOG_ERR("uart_init failed (err %d)", err);
      return false;
    }
    return true;
  }

  void run() override {
    LOG_INF("[uart thread] starting");

    buf = static_cast<uart_data_t *>(k_fifo_get(&uart_rx_fifo, K_FOREVER));
    LOG_INF("[uart thread] buf->data: %s", buf->data);
    
    size_t bytes_written;
    int err = k_pipe_put(&uart_nus_pipe, buf->data, buf->len, &bytes_written, 0, K_NO_WAIT);

    if (err < 0) {
      LOG_WRN("UART put to uart_nus_pipe failed: %d", err);
    } else if (bytes_written < buf->len) {
      LOG_WRN("UART put to uart_nus_pipe incomplete: %d of %d bytes written", bytes_written, buf->len);
    }

    k_free(buf);
    LOG_INF("[uart thread] done");
  }

public:
  UartThread() {
    // Allocate memory for the buffer. Must use heap memory because the buffer is passed to NUS service.
    buf = static_cast<uart_data_t*>(k_malloc(sizeof(*buf)));
    if (!buf) {
      LOG_ERR("Not able to allocate UART receive buffer");
      return;
    }
  }

  ~UartThread() {
    k_free(buf);
  }

private:
  // Buffer used to store data received on NUS pipe and sent to BLE.
  struct uart_data_t *buf;
};

static void uart_thread_function(void *arg0, void *arg1, void *arg2) {
  ARG_UNUSED(arg0);
  ARG_UNUSED(arg1);
  ARG_UNUSED(arg2);

  UartThread thread;
  thread();
}

K_THREAD_DEFINE(uart_thread_id, CONFIG_MAIN_STACK_SIZE, uart_thread_function, NULL, NULL, NULL, 4, 0, 0);