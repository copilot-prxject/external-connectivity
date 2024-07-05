#pragma once

#include <copilot/BleConsts.h>

#include <map>

#include "TheThingsNetwork.h"

namespace extcon {

const std::map<port_t, uint16_t> portToUuid{
    {1, GATT_CHR_VOLTAGE_MEASUREMENT_CONTROL},
    {2, GATT_CHR_CURRENT_MEASUREMENT_CONTROL},
    {3, GATT_CHR_PWM},
    {4, GATT_CHR_RELAY},
    {5, GATT_CHR_TEMPERATURE_CONTROL},
};

}  // namespace extcon