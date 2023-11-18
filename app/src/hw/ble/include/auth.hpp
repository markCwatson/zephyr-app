#ifndef _AUTH_HPP_
#define _AUTH_HPP_

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>

class Auth {
public:
  // Public method to access the instance of the class.
  static Auth& get_instance() {
    // Guaranteed to be destroyed and instantiated on first use.
    static Auth instance;
    return instance;
  }

  // Delete copy constructor and copy assignment operator to prevent duplicates.
  Auth(const Auth&) = delete;
  Auth& operator=(const Auth&) = delete;

  // Insance methods
  int init();
  void unref();

  // Public static callbacks
  // \todo: in order to use and register these callbacks, I have to set CONFIG_BT_SMP=y
  //        See proj.config for more info
  static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey);
  static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey);
  static void auth_cancel(struct bt_conn *conn);
  static void pairing_complete(struct bt_conn *conn, bool bonded);
  static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason);
private:
  // Private constructor for singleton pattern.
  Auth() : auth_conn(nullptr) {};

  // The connection object
  struct bt_conn *auth_conn;
};

#endif // _AUTH_HPP_