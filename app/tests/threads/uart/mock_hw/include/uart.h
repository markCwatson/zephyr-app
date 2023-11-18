/**
 * @file uart.h
 * 
 * @brief UART mock hardware header file.
 * 
 * This file contains the UART mock hardware data structure.
 * The test project is a C project so we need to use C linkage.
*/
#ifndef _UART_H_
#define _UART_H_
 
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// \todo: I don't like how data is duplicated here from main project.
//        Find another way to share data between main project and test project.

/**
 * @brief UART payload buffer element size.
*/
#define UART_BUF_SIZE 20

/**
 * @brief UART data structure.
*/
struct uart_data_t {
  void *fifo_reserved;
  uint8_t data[UART_BUF_SIZE];
  uint16_t len;
};

#ifdef __cplusplus
}

#endif
#endif /* _UART_H_ */