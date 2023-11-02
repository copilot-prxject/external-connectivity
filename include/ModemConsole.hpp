#pragma once

#include <esp_console.h>

#include <GsmService.hpp>
#include <cxx_include/esp_modem_primitives.hpp>

namespace repl {
using gsm::GsmService;

class ModemConsole {
public:
    ModemConsole(GsmService* gsmService);

    void start();
    void waitForExit();

private:
    esp_console_repl_t* repl;
    esp_console_repl_config_t replConfig;
    esp_console_dev_uart_config_t uartConfig;

    GsmService* gsmService;

    esp_modem::SignalGroup exitSignal{};
};
}  // namespace repl