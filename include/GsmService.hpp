#pragma once

#include <UartController.hpp>

using uart::UartController;

namespace gsm {

using AtCommand = std::string;

enum class NetworkStatus {
    NotRegistered = 0,
    RegisteredHome = 1,
    Searching = 2,
    RegistrationDenied = 3,
    Unknown = 4,
    RegisteredRoaming = 5,
};

struct Response {
    bool success;
    std::string content;
};

class GsmController {
public:
    GsmController(UartController&& uart);

    void eventLoop();

    void sendAtCommand(const AtCommand& command);
    Response getResponse();

    bool moduleConnected();
    bool networkConnected();
    bool gprsConnected();

    void connectGprs();

private:
    UartController& uart;
};

}  // namespace gsm