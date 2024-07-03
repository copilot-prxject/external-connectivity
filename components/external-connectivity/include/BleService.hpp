#pragma once

#include <string>

namespace ble {

class BleService {
public:
    BleService() = default;

    static void setValue(uint8_t index, const std::string &value);

    bool init();
    void start();

private:
    static void loop(void *);
};

}  // namespace ble