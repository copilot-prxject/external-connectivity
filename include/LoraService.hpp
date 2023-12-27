#pragma once

#include <string>

namespace lora {
class LoraService {
public:
    LoraService(std::string appEui, std::string appKey, std::string devEui);

private:
    std::string appEui;
    std::string appKey;
    std::string devEui;
};
}  // namespace lora