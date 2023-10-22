#pragma once

#include <UartController.hpp>

using uart::UartController;

namespace gsm {

using AtCommand = std::string;

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

    void eventLoop();

    Result sendCommandGetResult(const AtCommand& command);

    bool moduleConnected();
    bool networkConnected();
    bool gprsConnected();

    Result connectGprs();

private:
    void sendAtCommand(const AtCommand& command);
    Response getResponse();

    UartController& uart;
};

}  // namespace gsm