#pragma once

#include <string>

#include "host/ble_hs.h"
// Comment to avoid sorting includes due to `esp_central.h` external dependency
#include "esp_central.h"
#include "host/ble_gap.h"
// Undefine `min` and `max` macros defined in the header above to avoid conflicts
#undef min
#undef max

namespace extcon::ble {

class BleService {
public:
    BleService() = default;

    static void setValue(uint8_t index, const std::string &value);

    bool init();
    void start();
};

void loop(void *);
void scanDevices();

void onReset(int reason);
void onSync();

int onEvent(ble_gap_event *event, void *arg);
int handleEventDiscovery(const ble_gap_event &event);
int handleEventConnect(const ble_gap_event &event);
int handleEventDisconnect(const ble_gap_event &event);
int handleEventNotifyDownlink(const ble_gap_event &event);

void tryConnecting(const ble_gap_event &event);
bool shouldConnect(const ble_gap_event &event);

void onDiscoveryComplete(const peer *peer, int status, void *arg);

}  // namespace extcon::ble