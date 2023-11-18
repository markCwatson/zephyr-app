/**
 * @file ble.cpp
 * 
 * @brief BLE implementation.
 * 
 * @details This file implements the BLE class. It is responsible for initializing the BLE device,
 *          starting advertising, and handling connection events.
*/
#include "ble.hpp"
#include "auth.hpp"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <bluetooth/services/nus.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ble_hw);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN	(sizeof(DEVICE_NAME) - 1)

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),
};

/**
 * @brief Callbacks for connection events.
*/
BT_CONN_CB_DEFINE(conn_callbacks) = {
  .connected = Ble::connected,
  .disconnected = Ble::disconnected,
};

/**
 * @brief Make the BLE instance available to the static callbacks/handlers.
*/
static Ble *ble_instance;

/**
 * @brief Advertisement parameters.
 * 
 * @Note Compiler complains about taking address of temporary array if BT_LE_ADV_CONN is used directly.
*/
static struct bt_le_adv_param *adv_param = BT_LE_ADV_CONN;

/**
 * @brief A callback for when a connection is established.
 * 
 * @details This function is called when a connection is established. It starts the MTU exchange
 *          and sets the security level to L2.
 * 
 * @param conn The connection object.
 * @param conn_err The connection error code.
*/
void Ble::connected(struct bt_conn *conn, uint8_t conn_err) {
  char addr[BT_ADDR_LE_STR_LEN];

  if (conn_err) {
    LOG_ERR("Connection failed (err %u)", conn_err);
    return;
  }

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
  LOG_INF("Connected %s", addr);

  // Update the internal state
  ble_instance->current_conn = bt_conn_ref(conn);
  ble_instance->state = {
    .is_initialized = ble_instance->state.is_initialized,
    .is_advertising = false,
    .is_connected = true,
  };

  // todo: post connection event to zbus
}

/**
 * @brief A callback for when a connection is disconnected.
 * 
 * @details This function is called when a connection is disconnected. It starts scanning again.
 * 
 * @param conn The connection object.
 * @param reason The reason for disconnection.
*/
void Ble::disconnected(struct bt_conn *conn, uint8_t reason) {
  char addr[BT_ADDR_LE_STR_LEN];

  // Converts binary LE Bluetooth address to string.
  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  LOG_INF("Disconnected: %s (reason %u)", addr, reason);

  // Update the internal state
  ble_instance->state.is_connected = false;

  // Unreference the connection object
  ble_instance->auth.unref();
  if (ble_instance->current_conn) {
    bt_conn_unref(ble_instance->current_conn);
    ble_instance->current_conn = nullptr;
  }

  // todo: post disconnection event to zbus
}

/**
 * @brief Start scanning for BLE devices.
 * 
 * @details This function starts scanning for BLE devices.
 * 
 * @return 0 if successful, negative errno otherwise.
*/
int Ble::start_advertising() {
  LOG_INF("Starting BLE advertising");

  // Start advertising, set advertisement data, scan response data, advertisement parameters, and start advertising.
	int err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		return err;
	}

  // Update the internal state
  this->state.is_advertising = true;
  LOG_INF("Advertising successfully started");
  return 0;
}

/**
 * @brief Initialize the BLE device.
 * 
 * @details This function initializes the BLE device. It registers the authorization and authorization
 *          info callbacks, enables the BLE device, and starts scanning.
 * 
 * @return 0 if successful, negative errno otherwise.
*/
int Ble::init() {
  int err;

  // Make the BLE instance available to the static methods.
  ble_instance = this;

  // Use the singleton instance of the Auth class to initialize it.
  err = ble_instance->auth.init();
  if (err) {
    LOG_ERR("Failed to initialize authorization module (err %d)", err);
    return -1;
  }

  // Enable the BLE device
  err = bt_enable(NULL);
  if (err) {
    LOG_ERR("Bluetooth init failed (err %d)", err);
    return -2;
  }

  // note: call to settings_load be after bt_enable
  // See https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/zephyr/connectivity/bluetooth/bluetooth-arch.html#persistent-storage
  if (IS_ENABLED(CONFIG_SETTINGS)) {
    settings_load();
  }

  // Update the internal state
  this->state.is_initialized = true;
  LOG_INF("Bluetooth initialized");

  return 0;
}