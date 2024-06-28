#pragma once

#include <esp_console.h>

#include <GsmService.hpp>
#include <HttpClient.hpp>
#include <LoraService.hpp>
#include <cxx_include/esp_modem_primitives.hpp>
#include <vector>

namespace repl {
using gsm::GsmService;
using http::HttpClient;
using lora::LoraService;

class CommandRegistry;

class ModemConsole {
public:
    ModemConsole(GsmService *gsmService, HttpClient *httpClient,
                 LoraService *loraService);
    void start();
    void waitForExit();

    esp_modem::SignalGroup exitSignal{};

private:
    esp_console_repl_t *repl;
    esp_console_repl_config_t replConfig;
    esp_console_dev_uart_config_t uartConfig;
    std::unique_ptr<CommandRegistry> commandRegistry;
};

class CommandRegistry {
public:
    CommandRegistry(ModemConsole *console, GsmService *gsmService,
                    HttpClient *httpClient, LoraService *loraService);
    void registerCommands();

private:
    static ModemConsole *console;
    static GsmService *gsmService;
    static HttpClient *httpClient;
    static LoraService *loraService;
    std::vector<esp_console_cmd_t> commands;
};
}  // namespace repl