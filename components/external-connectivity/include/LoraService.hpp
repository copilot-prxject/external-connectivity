#pragma once

#include <TheThingsNetwork.h>
#include <copilot/BleConsts.h>

#include <queue>
#include <string>

namespace extcon::lora {

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

}  // namespace extcon::lora