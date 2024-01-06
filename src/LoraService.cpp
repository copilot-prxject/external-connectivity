#include "LoraService.hpp"

#include <driver/gpio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <nvs_flash.h>

#define I2C_SDA 21
#define I2C_SCL 22
#define OLED_RST UNUSE_PIN
#define BOARD_LED 25

namespace lora {
constexpr auto logTag = "lora";

LoraService::LoraService(std::string appEui, std::string appKey, std::string devEui)
    : appEui{appEui}, appKey{appKey}, devEui{devEui} {
}

void messageReceived(const uint8_t* message, size_t length, ttn_port_t port) {
    ESP_LOGI(logTag, "Message of %d bytes received on port %d:", length, port);
    for (int i = 0; i < length; i++) ESP_LOGI(logTag, " %02x", message[i]);
}

bool LoraService::init() {
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));
    ESP_ERROR_CHECK(nvs_flash_init());

    spi_bus_config_t busConfig{};
    busConfig.mosi_io_num = GPIO_NUM_27;
    busConfig.miso_io_num = GPIO_NUM_19;
    busConfig.sclk_io_num = GPIO_NUM_5;
    ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &busConfig, SPI_DMA_DISABLED));

    ttn.configurePins(HSPI_HOST, GPIO_NUM_18, TTN_NOT_CONNECTED, GPIO_NUM_23, GPIO_NUM_26, GPIO_NUM_33);
    ttn.provision(devEui.c_str(), appEui.c_str(), appKey.c_str());
    ttn.onMessage(messageReceived);

    bool result = ttn.join();
    if (result) {
        ESP_LOGI(logTag, "Joined successfully");
    } else {
        ESP_LOGE(logTag, "Join failed");
    }

    return result;
}

void LoraService::start() {
    xTaskCreate(sendTask, "sendTask", 4096, static_cast<void*>(this), 1, nullptr);
}

void LoraService::sendTask(void* pvParameter) {
    LoraService* loraService = static_cast<LoraService*>(pvParameter);
    assert(loraService != nullptr);

    while (1) {
        uint8_t message[] = "Hello, world";
        ESP_LOGI(logTag, "Sending message...");
        TTNResponseCode res = loraService->ttn.transmitMessage(message, sizeof(message) - 1);
        ESP_LOGI(logTag, "%s", res == kTTNSuccessfulTransmission ? "Message sent." : "Transmission failed.");

        vTaskDelay(30 * pdMS_TO_TICKS(1000));
    }
}

}  // namespace lora