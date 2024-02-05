#pragma once

#include <TheThingsNetwork.h>

#include <string>

namespace lora {
class LoraService {
public:
    static void loop(void* parameters);
    static void onDownlinkMessage(const uint8_t* message, size_t length, port_t port);

    LoraService(std::string appEui, std::string appKey, std::string devEui);
    bool init();
    void start(uint32_t stackDepth);
    void joinNetwork();

private:
    const std::string appEui;
    const std::string appKey;
    const std::string devEui;

    TheThingsNetwork ttn{};
};
}  // namespace lora