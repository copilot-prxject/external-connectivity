#pragma once

#include <esp_console.h>

#include <GsmService.hpp>
#include <cxx_include/esp_modem_primitives.hpp>
#include <list>

namespace repl {
using gsm::GsmService;

class CommandRegistry;

class ModemConsole {
public:
    ModemConsole(std::unique_ptr<GsmService> gsmService);
    void start();
    void waitForExit();

    esp_modem::SignalGroup exitSignal{};

private:
    esp_console_repl_t *repl;
    esp_console_repl_config_t replConfig;
    esp_console_dev_uart_config_t uartConfig;
    std::unique_ptr<GsmService> gsmService;
    std::unique_ptr<CommandRegistry> commandRegistry;
};

class CommandRegistry {
public:
    CommandRegistry(ModemConsole *console, GsmService *gsmService);
    void registerCommands();

private:
    void registerCommand(esp_console_cmd_t command);

    static ModemConsole *console;
    static GsmService *gsmService;
    std::list<esp_console_cmd_t> commands{};
};
}  // namespace repl