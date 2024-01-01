#pragma once

#include <TheThingsNetwork.h>

#include <string>

namespace lora {
class LoraService {
public:
    LoraService(std::string appEui, std::string appKey, std::string devEui);

    void init();
    void send();

private:
    const std::string appEui;
    const std::string appKey;
    const std::string devEui;

    TheThingsNetwork ttn{};
};
}  // namespace lora