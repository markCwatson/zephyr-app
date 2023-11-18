#include "uart.h"

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

extern struct k_fifo uart_rx_fifo;
extern struct k_pipe uart_nus_pipe; 

/**
 * @brief Tests the uart thread.
 *
 * This test checks if the uart thread puts data into the pipe correctly.
 */
ZTEST(uart_thread, test_uart_pipe)
{
  struct uart_data_t *buf_out;
  struct uart_data_t *buf_in;

  // Allocate memory for buffers
  buf_out = k_malloc(sizeof(struct uart_data_t));
  buf_in = k_malloc(sizeof(struct uart_data_t));

  for (int i = 0; i < UART_BUF_SIZE; ++i) {
    // Put data into FIFO buffer
    buf_out->data[i] = i;
    buf_out->len = i + 1;
    k_fifo_put(&uart_rx_fifo, buf_out);

    // Get data from pipe buffer
    size_t bytes_read;
    zassert_true(0 == k_pipe_get(&uart_nus_pipe, buf_in->data, i + 1, &bytes_read, 0, K_FOREVER));

    // Check if data read from pipe buffer matches data put into FIFO buffer
    zassert_true(bytes_read == i + 1);
    zassert_true(buf_in->data[i] == buf_out->data[i]);
    zassert_true(buf_in->len == buf_out->len);
  }
}

ZTEST_SUITE(uart_thread, NULL, NULL, NULL, NULL, NULL);