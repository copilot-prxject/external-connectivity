#include "LoraService.hpp"

#include <driver/gpio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <nvs_flash.h>

namespace lora {
LoraService::LoraService(std::string appEui, std::string appKey, std::string devEui)
    : appEui{appEui}, appKey{appKey}, devEui{devEui} {
}

void LoraService::init() {
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));

    ESP_ERROR_CHECK(nvs_flash_init());

    spi_bus_config_t busConfig{};
    busConfig.mosi_io_num = GPIO_NUM_27;
    busConfig.miso_io_num = GPIO_NUM_19;
    busConfig.sclk_io_num = GPIO_NUM_5;
    ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &busConfig, SPI_DMA_DISABLED));

    ttn.configurePins(HSPI_HOST, GPIO_NUM_18, TTN_NOT_CONNECTED, GPIO_NUM_14, GPIO_NUM_26, GPIO_NUM_35);

    ttn.provision(devEui.c_str(), appEui.c_str(), appKey.c_str());

    if (ttn.join()) {
        ESP_LOGI("lora", "Joined successfully");
    } else {
        ESP_LOGE("lora", "Join failed");
    }
}

void LoraService::send() {
}

}  // namespace lora