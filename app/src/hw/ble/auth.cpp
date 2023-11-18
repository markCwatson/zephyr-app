#include "auth.hpp"
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ble_auth);

static struct bt_conn_auth_cb conn_auth_callbacks = {
  .passkey_display = Auth::auth_passkey_display,
  .passkey_confirm = Auth::auth_passkey_confirm,
  .cancel = Auth::auth_cancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
  .pairing_complete = Auth::pairing_complete,
  .pairing_failed = Auth::pairing_failed
};

/**
 * @brief Make the Auth instance available to the static callbacks/handlers.
*/
static Auth* auth_instance;

/**
 * @brief A callback for when a passkey is displayed.
 * 
 * @details This function is called when a passkey is displayed. It logs the passkey.
 * 
 * @param conn The connection object.
 * @param passkey The passkey.
*/
void Auth::auth_passkey_display(struct bt_conn *conn, unsigned int passkey) {
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  LOG_INF("Passkey for %s: %06u", addr, passkey);
}

/**
 * @brief A callback for when a passkey is confirmed.
 * 
 * @details This function is called when a passkey is confirmed. It logs the passkey.
 * 
 * @param conn The connection object.
 * @param passkey The passkey.
*/
void Auth::auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey) {
  char addr[BT_ADDR_LE_STR_LEN];

  auth_instance->auth_conn = bt_conn_ref(conn);

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  LOG_INF("Passkey for %s: %06u", addr, passkey);
  LOG_INF("Press Button 1 to confirm, Button 2 to reject.");
}

/**
 * @brief A callback for when pairing is cancelled.
 * 
 * @details This function is called when pairing is cancelled. It logs the address of the device.
 * 
 * @param conn The connection object.
*/
void Auth::auth_cancel(struct bt_conn *conn) {
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  LOG_INF("Pairing cancelled: %s", addr);
}

/**
 * @brief A callback for when pairing is completed.
 * 
 * @details This function is called when pairing is completed. It logs the address of the device.
 * 
 * @param conn The connection object.
 * @param bonded Whether the device is bonded.
*/
void Auth::pairing_complete(struct bt_conn *conn, bool bonded) {
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  LOG_INF("Pairing completed: %s, bonded: %d", addr, bonded);
}

/**
 * @brief A callback for when pairing fails.
 * 
 * @details This function is called when pairing fails. It logs the address of the device.
 * 
 * @param conn The connection object.
 * @param reason The reason for failure.
*/
void Auth::pairing_failed(struct bt_conn *conn, enum bt_security_err reason) {
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  LOG_INF("Pairing failed conn: %s, reason %d", addr, reason);
}

/**
 * @brief Unreference the connection object.
 * 
 * @details This function unreferences the connection object.
*/
void Auth::unref() {
  if (this->auth_conn) {
    bt_conn_unref(this->auth_conn);
    this->auth_conn = nullptr;
  }
}

/**
 * @brief Initialize the authorization module.
 * 
 * @details This function initializes the authorization module and registers the callbacks.
 * 
 * @return 0 if successful, otherwise negative error code.
*/
int Auth::init() {
  // Make the Auth instance available to the static methods.
  auth_instance = this;

  // Register authorization callbacks.
  int err = bt_conn_auth_cb_register(&conn_auth_callbacks);
  if (err) {
    LOG_ERR("Failed to register authorization callbacks.");
    return -1;
  }

  // Register authorization info callbacks.
  err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
  if (err) {
    LOG_ERR("Failed to register authorization info callbacks.\n");
    return -2;
  }

  return 0;
}