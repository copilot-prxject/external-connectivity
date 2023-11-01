#include "Console.hpp"

#include <numeric>

namespace repl {
Console::Console() {
    replConfig = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    uartConfig = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
}

void Console::start() {
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uartConfig, &replConfig, &repl));
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

void Console::waitForExit() {
    exitSignal.wait_any(1, std::numeric_limits<uint32_t>::max());
    repl->del(repl);
}
}  // namespace repl