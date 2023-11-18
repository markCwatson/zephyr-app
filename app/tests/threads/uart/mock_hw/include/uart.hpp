#ifndef _UART_MOCK_HPP_
#define _UART_MOCK_HPP_

#include "uart.h"
#include <cstdint>
#include <zephyr/kernel.h>

/**
 * @brief Mock for Uart class.
 * 
 * @details This mock is used to mock the Uart class.
 *          Only the init() method is mocked.
*/
class Uart {
public:
  Uart();
  int init();
};

Uart::Uart() {
}

/**
 * @brief Mock for Uart::init().
*/
int Uart::init() {
  return 0;
}

#endif // _UART_MOCK_HPP_