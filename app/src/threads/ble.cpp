#include "thread_base.hpp"
#include "ble.hpp"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ble_thread);

/**
 * @brief Semaphore used to signal that BLE is initialized.
*/
extern struct k_sem ble_init_done;

/**
 * @brief Thread for handling BLE.
 * 
 * @details This thread is responsible for initializing and managing the BLE.
*/
class BleThread : public ThreadBase<BleThread> {
  friend class ThreadBase<BleThread>;

protected:
  bool init() override {
    int err;

    // Get the singleton instance of the BLE class and initialize it.
    Ble& ble = Ble::get_instance();
    err = ble.init();
    if (err != 0) {
      LOG_ERR("ble.init failed (err %d)", err);
      return false;
    }

    // Signal that BLE is initialized.
    k_sem_give(&ble_init_done);

    // Start advertising BLE device.
    err = ble.start_advertising();
    if (err != 0) {
      LOG_ERR("ble.start_advertising failed (err %d)", err);
      return false;
    }

    return true;
  }

  void run() override {
    k_sleep(K_FOREVER);
  }
};

void ble_thread_function(void *arg0, void *arg1, void *arg2) {
  ARG_UNUSED(arg0);
  ARG_UNUSED(arg1);
  ARG_UNUSED(arg2);

  BleThread thread;
  thread();
}

K_THREAD_DEFINE(ble_thread_id, CONFIG_MAIN_STACK_SIZE, ble_thread_function, NULL, NULL, NULL, 4, 0, 0);