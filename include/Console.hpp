#pragma once

#include <esp_console.h>

#include <cxx_include/esp_modem_primitives.hpp>

namespace repl {
class Console {
public:
    Console();

    void start();
    void waitForExit();

private:
    esp_console_repl_t* repl;
    esp_console_repl_config_t replConfig;
    esp_console_dev_uart_config_t uartConfig;

    esp_modem::SignalGroup exitSignal{};
};
}  // namespace repl