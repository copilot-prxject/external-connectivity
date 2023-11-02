#include "ModemConsole.hpp"

#include <numeric>

namespace repl {
constexpr auto logTag = "repl";

ModemConsole::ModemConsole(std::unique_ptr<GsmService> gsmService) : gsmService{std::move(gsmService)} {
    replConfig = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    uartConfig = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
}

void ModemConsole::start() {
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uartConfig, &replConfig, &repl));
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

void ModemConsole::waitForExit() {
    exitSignal.wait_any(1, std::numeric_limits<uint32_t>::max());
    repl->del(repl);
}
}  // namespace repl