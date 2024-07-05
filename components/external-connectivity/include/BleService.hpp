#pragma once

#include <string>

#include "InternalMappings.hpp"
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

    static void writeValue(Uuid uuid, const std::string &value);

    bool init();
    void start();

    static const peer *connectedPeer;

private:
    static void loop(void *);
    static void scanDevices();

    static void onReset(int reason);
    static void onSync();

    static int onEvent(ble_gap_event *event, void *arg);
    static int handleEventDiscovery(const ble_gap_event &event);
    static int handleEventConnect(const ble_gap_event &event);
    static int handleEventDisconnect(const ble_gap_event &event);
    static int handleEventNotifyDownlink(const ble_gap_event &event);

    static void tryConnecting(const ble_gap_event &event);
    static bool shouldConnect(const ble_gap_event &event);

    static void onDiscoveryComplete(const peer *peer, int status, void *arg);
    static void subscribeToNotifications(const peer &peer);
    static void subscribe(const peer &peer, const peer_chr &characteristic);
    static void write(uint16_t valueHandle, const std::string &value);
};

}  // namespace extcon::ble