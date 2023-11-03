#include "ModemConsole.hpp"

#include <numeric>

namespace repl {
constexpr auto logTag = "repl";

ModemConsole::ModemConsole(std::unique_ptr<GsmService> gsmService)
    : gsmService{std::move(gsmService)},
      commandRegistry{std::make_unique<CommandRegistry>(this, this->gsmService.get())} {
    replConfig = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    uartConfig = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uartConfig, &replConfig, &repl));
}

void ModemConsole::start() {
    commandRegistry->registerCommands();
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

void ModemConsole::waitForExit() {
    exitSignal.wait_any(1, std::numeric_limits<uint32_t>::max());
    repl->del(repl);
}

CommandRegistry::CommandRegistry(ModemConsole *console, GsmService *gsmService) {
    CommandRegistry::console = console;
    CommandRegistry::gsmService = gsmService;
}

void CommandRegistry::registerCommands() {
    registerCommand({"exit", "Exits the console", nullptr,
                     [](int, char **) {
                         console->exitSignal.set(1);
                         return 0;
                     },
                     nullptr});
}

void CommandRegistry::registerCommand(esp_console_cmd_t command) {
    commands.push_back(command);
    ESP_ERROR_CHECK(esp_console_cmd_register(&commands.back()));
}

ModemConsole *CommandRegistry::console{};
GsmService *CommandRegistry::gsmService{};
}  // namespace repl