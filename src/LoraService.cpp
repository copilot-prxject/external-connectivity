#include "LoraService.hpp"

namespace lora {
LoraService::LoraService(std::string appEui, std::string appKey, std::string devEui)
    : appEui{appEui}, appKey{appKey}, devEui{devEui} {
}
}  // namespace lora