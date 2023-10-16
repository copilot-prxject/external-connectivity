#pragma once

#include <UartController.hpp>

using uart::UartController;

namespace gsm {

class GsmController {
public:
    GsmController(UartController&& uart);

    void eventLoop();
    bool checkConnection();

private:
    UartController& uart;
};

}  // namespace gsm