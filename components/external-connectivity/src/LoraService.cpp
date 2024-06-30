#include <driver/gpio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>

#include <BleService.hpp>

#include "LoraService.hpp"

namespace lora {
constexpr auto logTag = "lora";

void LoraService::loop(void* pvParameter) {
    LoraService* loraServiceHandle = static_cast<LoraService*>(pvParameter);
    assert(loraServiceHandle != nullptr);

    ESP_LOGI(logTag, "Joining network...");
    loraServiceHandle->joinNetwork();

    while (true) {
        uint8_t message[] = "sample_type,sample_value";
        ESP_LOGI(logTag, "Sending message...");
        TTNResponseCode res = loraServiceHandle->ttn.transmitMessage(message, sizeof(message) - 1);
        ESP_LOGI(logTag, "%s", res == kTTNSuccessfulTransmission ? "Message sent." : "Transmission failed.");

        vTaskDelay(30 * pdMS_TO_TICKS(1000));
    }
}

void LoraService::onDownlinkMessage(const uint8_t* message, size_t length, port_t port) {
    if (length == 0) {
        ESP_LOGI(logTag, "Empty message received.");
        return;
    }
    std::string messageString{reinterpret_cast<const char*>(message), length};
    ESP_LOGI(logTag, "Message received: \"%s\", length: %d, port: %d", messageString.c_str(), length, port);
    ble::BleService::setValue(port - 1, messageString);
}

LoraService::LoraService(std::string appEui, std::string appKey, std::string devEui)
    : appEui{appEui}, appKey{appKey}, devEui{devEui} {
}

bool LoraService::init() {
    spi_bus_config_t busConfig{};
    busConfig.mosi_io_num = GPIO_NUM_27;
    busConfig.miso_io_num = GPIO_NUM_19;
    busConfig.sclk_io_num = GPIO_NUM_5;
    ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &busConfig, SPI_DMA_DISABLED));

    ttn.configurePins(HSPI_HOST, GPIO_NUM_18, TTN_NOT_CONNECTED, GPIO_NUM_23, GPIO_NUM_26, GPIO_NUM_33);
    ttn.provision(devEui.c_str(), appEui.c_str(), appKey.c_str());
    ttn.onMessage(onDownlinkMessage);

    ESP_LOGI(logTag, "LoRa service initialized.");
    return true;
}

void LoraService::start(bool asTask, uint32_t stackDepth) {
    ESP_LOGI(logTag, "LoRa service started.");
    if (!asTask) {
        loop(this);
        return;
    }
    xTaskCreate(loop, "loraLoop", stackDepth, static_cast<void*>(this), 1, nullptr);
}

void LoraService::joinNetwork() {
    bool success = ttn.join();
    while (!success) {
        ESP_LOGE(logTag, "Join failed, retrying in 30 seconds...");
        vTaskDelay(30 * pdMS_TO_TICKS(1000));
        success = ttn.join();
    }
    ESP_LOGI(logTag, "Joined network.");
}
}  // namespace lora