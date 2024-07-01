#pragma once

#include <TheThingsNetwork.h>
#include <copilot/BleConsts.h>

#include <map>
#include <queue>
#include <string>

namespace lora {

const std::map<port_t, uint16_t> portToUuid{
    {1, GATT_CHR_VOLTAGE_MEASUREMENT_CONTROL},
    {2, GATT_CHR_CURRENT_MEASUREMENT_CONTROL},
    {3, GATT_CHR_PWM},
    {4, GATT_CHR_RELAY},
};

class LoraService {
public:
    static bool networkJoined;
    static std::queue<std::string> uplinkQueue;

    static void loop(void *parameters);
    static void onDownlinkMessage(const uint8_t *message, size_t length, port_t port);
    static void sendUplinkMessage(const std::string &message);

    LoraService(std::string appEui, std::string appKey, std::string devEui);
    bool init();
    void start(bool asTask);
    void joinNetwork();

private:
    const std::string appEui;
    const std::string appKey;
    const std::string devEui;

    TheThingsNetwork ttn{};
};

}  // namespace lora