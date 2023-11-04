#include "GsmService.hpp"

namespace gsm {
constexpr auto logTag = "gsm";

GsmService::GsmService(const char* apn) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    configure(apn);

    dte = esp_modem::create_uart_dte(&dteConfig);
    dce = esp_modem::create_SIM800_dce(&dceConfig, dte, netif);
    assert(dce != nullptr);
}

void GsmService::configure(const char* apn) {
    dceConfig = {.apn = apn};
    netifConfig = ESP_NETIF_DEFAULT_PPP();
    netif = esp_netif_new(&netifConfig);

    dteConfig = ESP_MODEM_DTE_DEFAULT_CONFIG();
    dteConfig.task_stack_size = 4096;
    dteConfig.task_priority = 5;
    dteConfig.dte_buffer_size = 512;

    auto& uartConfig = dteConfig.uart_config;
    uartConfig.tx_io_num = CONFIG_EXT_CON_UART_TX_PIN;
    uartConfig.rx_io_num = CONFIG_EXT_CON_UART_RX_PIN;
    uartConfig.rts_io_num = CONFIG_EXT_CON_UART_RTS_PIN;
    uartConfig.cts_io_num = CONFIG_EXT_CON_UART_CTS_PIN;
    uartConfig.rx_buffer_size = 1024;
    uartConfig.tx_buffer_size = 512;
    uartConfig.flow_control = ESP_MODEM_FLOW_CONTROL_NONE;
    uartConfig.event_queue_size = 30;
}

}  // namespace gsm