#pragma once

#include <host/ble_gatt.h>
#include <host/ble_hs.h>
#include <host/ble_uuid.h>
#include <host/util/util.h>

// Workaround for `min` and `max` defines as macros in headers above
#undef min
#undef max

#include <list>
#include <string>

namespace ble {

struct GattCharacteristic {
    ble_gatt_chr definition;
    ble_gatt_dsc descriptor;
};

struct GattService {
    ble_gatt_svc definition;
    std::list<GattCharacteristic> characteristics;
};

struct Peer {
    uint16_t connectionHandle;
    std::list<GattService> services;
    uint16_t previousCharacteristicHandle;
    GattService *currentService;
};

class BleService {
public:
    BleService() = default;

    static void setValue(uint8_t index, const std::string &value);

    bool init();
    void start();

private:
    static void loop(void *);

    static void onReset(int reason);
    static void onSync();
    static void scan();

    static int onEvent(ble_gap_event *event, void *arg);
    static void tryConnecting(const ble_gap_disc_desc &advertisingReport);
    static bool shouldConnect(const ble_gap_disc_desc &advertisingReport);

    static void discoverServices();
    static int onServiceDiscovery(uint16_t connection, const ble_gatt_error *error,
                                  const ble_gatt_svc *service, void *arg);
    static void discoverCharacteristics();
    static int onCharacteristicDiscovery(uint16_t connection,
                                         const ble_gatt_error *error,
                                         const ble_gatt_chr *characteristic, void *arg);

    static void discoverDescriptors();
    static int onDescriptorDiscovery(uint16_t connection, const ble_gatt_error *error,
                                     uint16_t chr_val_handle,
                                     const ble_gatt_dsc *descriptor, void *arg);

    static void completeDiscovery();
    static void terminateConnection();

    static void testWrite();
    static void testSubscribe();
};

}  // namespace ble