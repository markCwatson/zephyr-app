/**
 * @file uart.c
 *
 * @note This file needs to be C for the test project to compile.
 */
#include <zephyr/kernel.h>

/**
 * @brief Pipes used by the UART.
*/
K_PIPE_DEFINE(nus_uart_pipe, 1024, 4);

/**
 * @brief FIFO buffer for UART data.
*/
K_FIFO_DEFINE(uart_rx_fifo);