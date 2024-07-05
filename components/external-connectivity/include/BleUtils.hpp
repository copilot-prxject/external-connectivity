#pragma once

#include <list>

#include "host/ble_gap.h"
// Workaround for `min` and `max` defines as macros in headers above
#undef min
#undef max

namespace extcon::ble {

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

void onReset(int reason);
void onSync();
void scan();

int onEvent(ble_gap_event *event, void *arg);
void tryConnecting(const ble_gap_disc_desc &advertisingReport);
bool shouldConnect(const ble_gap_disc_desc &advertisingReport);

void discoverServices();
int onServiceDiscovery(uint16_t connection, const ble_gatt_error *error,
                       const ble_gatt_svc *service, void *arg);
void discoverCharacteristics();
int onCharacteristicDiscovery(uint16_t connection, const ble_gatt_error *error,
                              const ble_gatt_chr *characteristic, void *arg);

void discoverDescriptors();
int onDescriptorDiscovery(uint16_t connection, const ble_gatt_error *error,
                          uint16_t chr_val_handle, const ble_gatt_dsc *descriptor,
                          void *arg);

void completeDiscovery();
void terminateConnection();

void testWrite();
void testSubscribe();

GattService *getService(uint16_t serviceUuid);
GattService *getServiceByHandle(uint16_t handle);
GattCharacteristic *getCharacteristic(uint16_t serviceUuid, uint16_t characteristicUuid);

}  // namespace extcon::ble