#pragma once

#include <UartController.hpp>

using uart::UartController;

namespace gsm {

using AtCommand = std::string;

struct Response {
    bool valid;
    std::string response;
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