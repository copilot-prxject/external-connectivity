#include "ModemConsole.hpp"

#include <magic_enum.hpp>
#include <map>
#include <numeric>

namespace repl {
using esp_modem::modem_mode;

constexpr auto logTag = "repl";

ModemConsole *CommandRegistry::console{};
GsmService *CommandRegistry::gsmService{};
HttpClient *CommandRegistry::httpClient{};

ModemConsole::ModemConsole(GsmService *gsmService, HttpClient *httpClient)
    : commandRegistry{std::make_unique<CommandRegistry>(this, gsmService, httpClient)} {
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

CommandRegistry::CommandRegistry(ModemConsole *console, GsmService *gsmService, HttpClient *httpClient) {
    CommandRegistry::console = console;
    CommandRegistry::gsmService = gsmService;
    CommandRegistry::httpClient = httpClient;
}

void CommandRegistry::registerCommands() {
    commands = {
        {"exit", "Exits the console", nullptr,
         [](int, char **) {
             console->exitSignal.set(1);
             return ESP_OK;
         },
         nullptr},
        {"mode", "Sets the modem mode", "<PPP|CMD>",
         [](int argc, char **argv) {
             std::map<std::string, modem_mode> mode{
                 {"PPP", modem_mode::DATA_MODE},
                 {"CMD", modem_mode::COMMAND_MODE},
             };
             if (argc != 2 || !mode.contains(argv[1])) {
                 return ESP_ERR_INVALID_ARG;
             }
             gsmService->dce->set_mode(mode[argv[1]]);
             ESP_LOGI(logTag, "Modem mode set to %s", argv[1]);
             return ESP_OK;
         },
         nullptr},
        {"signal", "Gets the signal strength", nullptr,
         [](int, char **) {
             int rssi, ber;
             gsmService->dce->get_signal_quality(rssi, ber);
             ESP_LOGI(logTag, "Signal strength: %d, BER: %d", rssi, ber);
             return ESP_OK;
         },
         nullptr},
        {"operator", "Gets the operator name", nullptr,
         [](int, char **) {
             std::string operatorName;
             int access;
             gsmService->dce->get_operator_name(operatorName, access);
             ESP_LOGI(logTag, "Operator name: %s, access: %d", operatorName.c_str(), access);
             return ESP_OK;
         },
         nullptr},
        {"get", "Performs a GET request", "<url>",
         [](int argc, char **argv) {
             if (argc != 2) {
                 return ESP_ERR_INVALID_ARG;
             }
             httpClient->get(argv[1]);
             return ESP_OK;
         },
         nullptr},
        {"post", "Performs a POST request", "<url> <data>",
         [](int argc, char **argv) {
             if (argc < 3) {
                 return ESP_ERR_INVALID_ARG;
             }
             std::map<std::string, std::string> data;
             for (int i = 2; i < argc; i++) {
                 std::string arg{argv[i]};
                 auto pos = arg.find('=');
                 if (pos == std::string::npos) {
                     return ESP_ERR_INVALID_ARG;
                 }
                 data[arg.substr(0, pos)] = arg.substr(pos + 1);
             }
             httpClient->post(argv[1], data);
             return ESP_OK;
         },
         nullptr},
        {"reset", "Resets the modem", nullptr,
         [](int, char **) {
             std::string output;
             gsmService->dce->at("AT+CFUN=1,1", output, 500);
             ESP_LOGI(logTag, "Modem reset");
             return ESP_OK;
         },
         nullptr},
    };

    std::for_each(commands.begin(), commands.end(),
                  [](auto &command) { ESP_ERROR_CHECK(esp_console_cmd_register(&command)); });
}
}  // namespace repl