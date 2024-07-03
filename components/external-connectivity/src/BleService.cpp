#include "BleService.hpp"

#include <copilot/BleConsts.h>

#include <BleUtils.hpp>
#include <LoraService.hpp>
#include <string>

#include "esp_err.h"
#include "esp_log.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"

namespace {

constexpr auto logTag{"ble"};
constexpr auto deviceName{"external-connectivity"};

}  // namespace

namespace ble {

void BleService::setValue(uint8_t index, const std::string &value) {
    ESP_LOGI(logTag, "Setting value for characteristic %d: %s", index, value.c_str());
}

bool BleService::init() {
    esp_log_level_set(logTag, ESP_LOG_DEBUG);

    ESP_ERROR_CHECK(nimble_port_init());

    ble_hs_cfg.gatts_register_cb = nullptr;
    ble_hs_cfg.reset_cb = onReset;
    ble_hs_cfg.sync_cb = onSync;

    ESP_ERROR_CHECK(ble_svc_gap_device_name_set(deviceName));

    return true;
}

void BleService::start() {
    ESP_LOGI(logTag, "Starting BleService.");
    nimble_port_freertos_init(loop);
}

void BleService::loop(void *) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

}  // namespace ble
