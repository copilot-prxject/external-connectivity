#pragma once

#include <esp_modem_config.h>

#include <cxx_include/esp_modem_api.hpp>
#include <cxx_include/esp_modem_dte.hpp>

namespace gsm {
class GsmService {
public:
    GsmService(const char* apn);

    std::unique_ptr<esp_modem::DCE> dce;

private:
    void configure(const char* apn);

    std::shared_ptr<esp_modem::DTE> dte;
    esp_modem_dce_config dceConfig;
    esp_modem_dte_config dteConfig;
    esp_netif_config netifConfig;
    esp_netif_t* netif;
};
}  // namespace gsm