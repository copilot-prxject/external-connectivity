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

    assert(statusIndex + 1 < response.size());
    const auto status = response.substr(statusIndex + 1, 1);
    if (!std::isdigit(status[0])) {
        ESP_LOGE(logTag, "%s: Invalid status: %s", __func__, status.c_str());
        return NetworkStatus::Unknown;
    }

    return magic_enum::enum_cast<NetworkStatus>(std::stoi(status)).value_or(NetworkStatus::Unknown);
}

bool checkNetworkStatus(const Response& response) {
    const auto status = getNetworkStatus(response.content);
    return response.type == Result::Ok &&
           (status == NetworkStatus::RegisteredHome || status == NetworkStatus::RegisteredRoaming);
}
}  // namespace

bool operator!(Result result) {
    return result != Result::Ok;
}

GsmController::GsmController(UartController&& uart) : uart(uart) {
    const int ledDisabled = 0;
    if (!setNetlightIndication(ledDisabled)) {
        ESP_LOGE(logTag, "%s: Failed to disable the netlight indication", __func__);
    }
}

void GsmController::eventLoop() {
    while (true) {
        if (!moduleConnected()) {
            ESP_LOGW(logTag, "%s: No connection to the GSM module", __func__);
            continue;
        }
        if (!networkConnected()) {
            ESP_LOGW(logTag, "%s: No connection to the network", __func__);
            continue;
        }
        if (!gprsConnected()) {
            ESP_LOGI(logTag, "%s: Connecting to GPRS", __func__);
            if (connectGprs() != Result::Ok) {
                ESP_LOGE(logTag, "%s: Failed to connect to GPRS", __func__);
                continue;
            }
        }
    }
}

Result GsmController::sendCommandGetResult(const AtCommand& command) {
    sendAtCommand(command);
    return getResponse().type;
}

void GsmController::sendAtCommand(const AtCommand& command) {
    uart.write(command + '\r');
}

Response GsmController::getResponse() {
    const auto responseString = uart.read();
    const auto responseType = (responseString.contains("OK"))
                                  ? Result::Ok
                                  : (responseString.contains("ERROR") ? Result::Error : Result::Unknown);
    return Response{
        .content = responseString,
        .type = responseType,
    };
}

bool GsmController::moduleConnected() {
    return sendCommandGetResult("AT") == Result::Ok;
}

bool GsmController::networkConnected() {
    sendAtCommand("AT+CREG?");
    return checkNetworkStatus(getResponse());
}

bool GsmController::gprsConnected() {
    sendAtCommand("AT+CGREG?");
    return checkNetworkStatus(getResponse());
}

Result GsmController::connectGprs() {
    const std::string apn = "internet";
    const std::string user = "";
    const std::string password = "";

    if (!sendCommandGetResult("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"")) return Result::Error;
    if (!sendCommandGetResult("AT+SAPBR=3,1,\"APN\",\"" + apn + "\"")) return Result::Error;
    if (!sendCommandGetResult("AT+SAPBR=3,1,\"USER\",\"" + user + "\"")) return Result::Error;
    if (!sendCommandGetResult("AT+SAPBR=3,1,\"PWD\",\"" + password + "\"")) return Result::Error;
    if (!sendCommandGetResult("AT+CGDCONT=1,\"IP\",\"" + apn + "\"")) return Result::Error;
    if (!sendCommandGetResult("AT+CGACT=1,1")) return Result::Error;
    if (!sendCommandGetResult("AT+SAPBR=1,1")) return Result::Error;
    if (!sendCommandGetResult("AT+SAPBR=2,1")) return Result::Error;
    if (!sendCommandGetResult("AT+CGATT=1")) return Result::Error;
    if (!sendCommandGetResult("AT+CIPMUX=1")) return Result::Error;
    if (!sendCommandGetResult("AT+CIPQSEND=1")) return Result::Error;
    if (!sendCommandGetResult("AT+CIPRXGET=1")) return Result::Error;
    if (!sendCommandGetResult("AT+CSTT=\"" + apn + "\",\"" + user + "\",\"" + password + "\"")) return Result::Error;
    if (!sendCommandGetResult("AT+CIICR")) return Result::Error;
    if (!sendCommandGetResult("AT+CIFSR;E0")) return Result::Error;
    if (!sendCommandGetResult("AT+CDNSCFG=\"8.8.8.8\",\"8.8.4.4\"")) return Result::Error;

    return Result::Ok;
}

Result GsmController::setNetlightIndication(int value) {
    if (!sendCommandGetResult("AT+CSGS=" + std::to_string(value))) return Result::Error;
    return Result::Ok;
}

}  // namespace gsm