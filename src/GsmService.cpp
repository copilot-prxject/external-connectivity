#include "GsmService.hpp"

#include <esp_log.h>
#include <fmt/format.h>

#include <magic_enum.hpp>
#include <regex>

namespace gsm {
static constexpr auto logTag = "gsm";

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
    uart.responseCallback = [this](std::string data) { handleResponse(data); };

    const int ledDisabled = 0;
    if (!setNetlightIndication(ledDisabled)) {
        ESP_LOGE(logTag, "%s: Failed to disable the netlight indication", __func__);
    }
}

void GsmController::loop() {
    while (true) {
        sendAtCommand("AT+CREG?");
        vTaskDelay(pdMS_TO_TICKS(1000));
        //     if (!communicationReady()) {
        //         ESP_LOGW(logTag, "%s: Communication not ready", __func__);
        //         continue;
        //     }
    }
}

Result GsmController::sendCommandGetResult(const AtCommand& command) {
    sendAtCommand(command);
    return getResponse().type;
}

void GsmController::sendAtCommand(const AtCommand& command) {
    uart.write(command + '\r');
    xSemaphoreTake(uart.semaphore, pdMS_TO_TICKS(1000));
}

Response GsmController::getResponse(TickType_t timeout) {
    const auto responseString = uart.read();
    const auto responseType = (responseString.contains("OK"))
                                  ? Result::Ok
                                  : (responseString.contains("ERROR") ? Result::Error : Result::Unknown);
    return Response{
        .content = responseString,
        .type = responseType,
    };
}

void GsmController::handleResponse(std::string& response) {
    ESP_LOGD(logTag, "%s: Received response:\n%s", __func__, response.c_str());
}

bool GsmController::communicationReady() {
    if (!moduleConnected()) {
        ESP_LOGW(logTag, "%s: No connection to the GSM module", __func__);
        return false;
    }
    if (!networkConnected()) {
        ESP_LOGW(logTag, "%s: No connection to the network", __func__);
        return false;
    }
    if (!gprsConnected()) {
        ESP_LOGI(logTag, "%s: Connecting to GPRS", __func__);
        if (connectGprs() != Result::Ok) {
            ESP_LOGE(logTag, "%s: Failed to connect to GPRS", __func__);
            return false;
        }
    }
    return true;
}

bool GsmController::moduleConnected() {
    return sendCommandGetResult("AT") == Result::Ok;
}

bool GsmController::networkConnected() {
    sendAtCommand("AT+CREG?");
    return checkNetworkStatus(getResponse());
}

bool GsmController::gprsConnected() {
    sendAtCommand("AT+CIFSR");
    const auto localIp = getLocalIp();
    if (localIp.empty()) {
        ESP_LOGE(logTag, "%s: Failed to get local IP", __func__);
        return false;
    }
    return true;
}

Address GsmController::getLocalIp() {
    const auto response = uart.read();
    const std::regex ipRegex(R"(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})");
    std::smatch match;
    if (!std::regex_search(response, match, ipRegex)) {
        ESP_LOGE(logTag, "%s: Failed to parse IP address from response: %s", __func__, response.c_str());
        return "";
    }

    return match.str();
}

Result GsmController::ping(const Address& host) {
    const int retryCount = 1;
    const int timeoutInTenthOfSeconds = 100;  // 100 * 0.1s = 100 * 100ms = 10s

    sendAtCommand(fmt::format("AT+CIPPING=\"{}\",{},{}", host, retryCount, timeoutInTenthOfSeconds));
    const auto response = getResponse();

    if (response.type != Result::Ok) {
        ESP_LOGE(logTag, "%s: Ping failed: %s", __func__, response.content.c_str());
        return Result::Error;
    }

    return Result::Ok;
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