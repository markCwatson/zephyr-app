#ifndef _BLE_HPP_
#define _BLE_HPP_

#include "hw_base.hpp"
#include "auth.hpp"
#include <errno.h>
#include <zephyr/kernel.h>
#include <bluetooth/services/nus.h>

/**
 * @brief BLE device.
 * 
 * @details This class is responsible for interfacing with the BLE subsystem.
 * 
 * @note This class is a singleton. Use the get_instance() method to access the instance.
*/
class Ble : public HwBase<Ble> {
public:
    // Public method to access the instance of the class.
    static Ble& get_instance() {
      // Guaranteed to be destroyed and instantiated on first use.
      static Ble instance;
      return instance;
    }

    // Delete copy constructor and copy assignment operator to prevent duplicates.
    Ble(const Ble&) = delete;
    Ble& operator=(const Ble&) = delete;

    // Initialize the BLE device.
    int init() override;

    // Start advertising BLE device.
    int start_advertising();

    // Public callbacks
    static void connected(struct bt_conn *conn, uint8_t conn_err);
    static void disconnected(struct bt_conn *conn, uint8_t reason);

private:
    // Private constructor for singleton pattern.
    Ble() : current_conn(nullptr), state{false, false, false}, auth(Auth::get_instance()) {};

    // The BLE connection object
    struct bt_conn *current_conn;

    // Internal states for the BLE device
    struct BleState {
      bool is_initialized;
      bool is_advertising;
      bool is_connected;
    };

    BleState state;

    // A reference to the auth instance
    Auth& auth;
};

#endif // _BLE_HPP_