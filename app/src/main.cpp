#include "uart.hpp"
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app_main);

/**
 * @brief Semaphore used to signal that BLE is initialized.
*/
K_SEM_DEFINE(ble_init_done, 0, 1);

int main(void) {
  LOG_INF("[main thread] begin");
  k_sleep(K_SECONDS(3));

  uint64_t counter = 0;

  while (true) {
    LOG_INF("[main thread] counter: %llu", ++counter);
    k_sleep(K_SECONDS(2));
  }

  return 0;
}
