#include "GsmService.hpp"

#include <esp_log.h>

#include <magic_enum.hpp>

namespace gsm {

static constexpr char logTag[] = "GSM";

namespace {
NetworkStatus getNetworkStatus(const std::string& response) {
    const auto statusIndex = response.find(',');
    if (statusIndex == std::string::npos) {
        ESP_LOGE(logTag, "%s: Invalid response: %s", __func__, response.c_str());
        return NetworkStatus::Unknown;
    }

    const auto status = response.substr(statusIndex + 1, 1);
    if (!std::isdigit(status[0])) {
        ESP_LOGE(logTag, "%s: Invalid status: %s", __func__, status.c_str());
        return NetworkStatus::Unknown;
    }

    return magic_enum::enum_cast<NetworkStatus>(std::stoi(status)).value_or(NetworkStatus::Unknown);
}

bool checkNetworkStatus(const Response& response) {
    const auto status = getNetworkStatus(response.content);
    return response.success && (status == NetworkStatus::RegisteredHome || status == NetworkStatus::RegisteredRoaming);
}
}  // namespace

GsmController::GsmController(UartController&& uart) : uart(uart) {
}

void GsmController::eventLoop() {
    while (true) {
        if (!moduleConnected()) {
            ESP_LOGW(logTag, "%s: GSM module is not connected", __func__);
            continue;
        }
        if (!networkConnected()) {
            ESP_LOGW(logTag, "%s: No connection to the network", __func__);
            continue;
        }
        if (!gprsConnected()) {
            ESP_LOGI(logTag, "%s: Connecting to GPRS", __func__);
            connectGprs();
        }
    }
}

void GsmController::sendAtCommand(const AtCommand& command) {
    uart.write(command + "\r");
}

Response GsmController::getResponse() {
    const auto responseString = uart.read();
    return Response{
        .success = responseString.contains("OK"),
        .content = responseString,
    };
}

bool GsmController::moduleConnected() {
    sendAtCommand("AT");
    return getResponse().success;
}

bool GsmController::networkConnected() {
    sendAtCommand("AT+CREG?");
    return checkNetworkStatus(getResponse());
}

bool GsmController::gprsConnected() {
    sendAtCommand("AT+CGREG?");
    return checkNetworkStatus(getResponse());
}

void GsmController::connectGprs() {
    const std::string apn = "internet";
    const std::string user = "";
    const std::string password = "";

    sendAtCommand("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
    getResponse();

    sendAtCommand("AT+SAPBR=3,1,\"APN\",\"" + apn + "\"");
    getResponse();

    sendAtCommand("AT+SAPBR=3,1,\"USER\",\"" + user + "\"");
    getResponse();

    sendAtCommand("AT+SAPBR=3,1,\"PWD\",\"" + password + "\"");
    getResponse();

    sendAtCommand("AT+CGDCONT=1,\"IP\",\"" + apn + "\"");
    getResponse();

    sendAtCommand("AT+CGACT=1,1");
    getResponse();

    sendAtCommand("AT+SAPBR=1,1");
    getResponse();

    sendAtCommand("AT+SAPBR=2,1");
    getResponse();

    sendAtCommand("AT+CGATT=1");
    getResponse();

    sendAtCommand("AT+CIPMUX=1");
    getResponse();

    sendAtCommand("AT+CIPQSEND=1");
    getResponse();

    sendAtCommand("AT+CIPRXGET=1");
    getResponse();

    sendAtCommand("AT+CSTT=\"" + apn + "\",\"" + user + "\",\"" + password + "\"");
    getResponse();

    sendAtCommand("AT+CIICR");
    getResponse();

    sendAtCommand("AT+CIFSR;E0");
    getResponse();

    sendAtCommand("AT+CDNSCFG=\"8.8.8.8\",\"8.8.4.4\"");
    getResponse();
}

}  // namespace gsm