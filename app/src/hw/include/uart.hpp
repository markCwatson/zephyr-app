#ifndef _UART_HPP_
#define _UART_HPP_

#include "hw_base.hpp"
#include <cstdint>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>

constexpr size_t UART_BUF_SIZE = 20;

struct uart_data_t {
  void *fifo_reserved;
  uint8_t data[UART_BUF_SIZE] {};
  uint16_t len{0};
};

// \todo: this should only be exposed to the uart thread
class Uart : public HwBase<Uart> {
  friend class HwBase<Uart>;

public:
  Uart() : dev_(DEVICE_DT_GET(DT_NODELABEL(uart0))) {}
  int init() override;

private:
  const struct device *dev_;
  struct k_work_delayable uart_work_;
  static void uart_callback(const struct device *dev, struct uart_event *evt, void *user_data);
  static void uart_work_handler(struct k_work *item);
};

#endif // _UART_HPP_