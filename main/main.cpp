#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <sdkconfig.h>

#include <BleService.hpp>
#include <GsmService.hpp>
#include <HttpClient.hpp>
#include <LoraService.hpp>
#include <ModemConsole.hpp>

constexpr auto logTag = "main";

std::unique_ptr<gsm::GsmService> gsmService;
std::unique_ptr<http::HttpClient> httpClient;
std::unique_ptr<lora::LoraService> loraService;
std::unique_ptr<ble::BleService> bleService;

extern "C" void app_main(void) {
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));
    ESP_ERROR_CHECK(nvs_flash_init());

    bleService = std::make_unique<ble::BleService>();
    if (!bleService->init()) {
        ESP_LOGE(logTag, "BLE service failed to initialize.");
        return;
    }
    bleService->start();

#ifdef CONFIG_EXT_CON_GSM_ENABLE
    gsmService = std::make_unique<gsm::GsmService>(CONFIG_EXT_CON_APN);
    httpClient = std::make_unique<http::HttpClient>();
#endif
#ifdef CONFIG_EXT_CON_LORA_ENABLE
    loraService = std::make_unique<lora::LoraService>(CONFIG_EXT_CON_LORA_APP_EUI,
                                                      CONFIG_EXT_CON_LORA_APP_KEY,
                                                      CONFIG_EXT_CON_LORA_DEV_EUI);
    if (!loraService->init()) {
        ESP_LOGE(logTag, "LoRa service failed to initialize.");
        return;
    }

    const bool launchAsTask{false};
#ifdef CONFIG_EXT_CON_REPL_ENABLE
    launchAsTask = true;
#endif
    loraService->start(taskStackDepth, launchAsTask);
#endif

#ifdef CONFIG_EXT_CON_REPL_ENABLE
    repl::ModemConsole console{gsmService.get(), httpClient.get(), loraService.get()};
    console.start();
    console.waitForExit();
#endif
}