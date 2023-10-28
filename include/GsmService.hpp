#pragma once

#include <UartController.hpp>

namespace gsm {
using uart::UartController;
using AtCommand = std::string;
using Address = std::string;

static constexpr auto defaultReadTimeout = pdMS_TO_TICKS(1000);

enum class Result {
    Ok,
    Error,
    Unknown,
};

bool operator!(Result result);

struct Response {
    std::string content;
    Result type;
};

enum class NetworkStatus {
    NotRegistered = 0,
    RegisteredHome = 1,
    Searching = 2,
    RegistrationDenied = 3,
    Unknown = 4,
    RegisteredRoaming = 5,
};

class GsmController {
public:
    GsmController(UartController&& uart);

    void loop();

private:
    Result sendCommandGetResult(const AtCommand& command);
    void sendAtCommand(const AtCommand& command);
    void handleResponse(std::string& line);

    bool communicationReady();
    bool moduleConnected();
    bool networkConnected();
    bool gprsConnected();

    Address getLocalIp();
    Result ping(const Address& host);
    Result connectGprs();
    Result setNetlightIndication(int value);

    UartController& uart;
    Response latestResponse;
};

}  // namespace gsm