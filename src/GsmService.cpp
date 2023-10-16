#include "GsmService.hpp"

#include <esp_log.h>

namespace gsm {

static constexpr char logTag[] = "GSM";

GsmController::GsmController(UartController&& uart) : uart(uart) {
    eventLoop();
}

void GsmController::eventLoop() {
    while (true) {
        ESP_LOGI(logTag, "%s: Checking connection = %d", __func__, checkConnection());
    }
}

bool GsmController::checkConnection() {
    uart.write("AT\r\n");
    const auto response = uart.read();
    if (response.find("OK") != std::string::npos) {
        ESP_LOGD(logTag, "%s: GSM module is connected", __func__);
        return true;
    }
    return false;
}

}  // namespace gsm