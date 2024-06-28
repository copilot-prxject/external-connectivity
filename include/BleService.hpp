#pragma once

#include <host/ble_hs.h>
#include <host/ble_uuid.h>
#include <host/util/util.h>

#include <array>
#include <string>

namespace ble {
class BleService {
public:
    static void setValue(uint8_t index, const std::string &value);
    static int onAccess(uint16_t conn_handle, uint16_t attr_handle, ble_gatt_access_ctxt *context, void *arg);

    BleService();
    bool init();
    void start();

private:
    static void loop(void *);
    static void onReset(int reason);
    static void onSync();
    static void onRegister(ble_gatt_register_ctxt *context, void *arg);
    static int onEvent(ble_gap_event *event, void *arg);
    static void advertise();
};
}  // namespace ble