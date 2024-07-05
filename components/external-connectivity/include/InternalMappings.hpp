#pragma once

#include <copilot/BleConsts.h>

#include <array>
#include <map>
#include <string>

#include "TheThingsNetwork.h"

namespace extcon {

using Uuid = uint16_t;

const std::map<port_t, Uuid> portToUuid{
    {1, GATT_CHR_VOLTAGE_MEASUREMENT},
    {2, GATT_CHR_CURRENT_MEASUREMENT},
    {3, GATT_CHR_PWM},
    {4, GATT_CHR_RELAY},
    {5, GATT_CHR_TEMPERATURE},
};

const std::array subscribableCharacteristics{
    GATT_CHR_CURRENT_MEASUREMENT,
    GATT_CHR_VOLTAGE_MEASUREMENT,
    GATT_CHR_PWM,
    GATT_CHR_RELAY,
    GATT_CHR_TEMPERATURE,
};

const std::map<Uuid, std::string> uuidToType{
    {GATT_CHR_ENGINE_SPEED, "engine_speed"},
    {GATT_CHR_FUEL_TANK_LEVEL, "fuel_tank_level"},
    {GATT_CHR_BATTERY_VOLTAGE, "battery_voltage"},
    {GATT_CHR_THROTTLE_POSITION, "throttle_position"},
    {GATT_CHR_CURRENT_MEASUREMENT, "current_measurement"},
    {GATT_CHR_VOLTAGE_MEASUREMENT, "voltage_measurement"},
    {GATT_CHR_TEMPERATURE, "temperature"},
};

}  // namespace extcon