#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

namespace esphome {
namespace daikin_brc1 {

// Values for Daikin BRC4CXXX IR Controllers
// Temperature
const uint8_t DAIKIN_BRC_TEMP_MIN_F = 60;                                // fahrenheit
const uint8_t DAIKIN_BRC_TEMP_MAX_F = 90;                                // fahrenheit
const float DAIKIN_BRC_TEMP_MIN_C = (DAIKIN_BRC_TEMP_MIN_F - 32) / 1.8;  // fahrenheit
const float DAIKIN_BRC_TEMP_MAX_C = (DAIKIN_BRC_TEMP_MAX_F - 32) / 1.8;  // fahrenheit

// Modes
const uint8_t DAIKIN_BRC_MODE_AUTO = 0x0a;
const uint8_t DAIKIN_BRC_MODE_COOL = 0x02;
const uint8_t DAIKIN_BRC_MODE_HEAT = 0x08;
const uint8_t DAIKIN_BRC_MODE_DRY = 0x01;
const uint8_t DAIKIN_BRC_MODE_FAN = 0x04;
const uint8_t DAIKIN_BRC_MODE_OFF = 0x00;
const uint8_t DAIKIN_BRC_MODE_ON = 0x80;

// Fan Speed
const uint8_t DAIKIN_BRC_FAN_1 = 0x80;
const uint8_t DAIKIN_BRC_FAN_2 = 0x40;
const uint8_t DAIKIN_BRC_FAN_3 = 0x20;
const uint8_t DAIKIN_BRC_FAN_AUTO = 0x10;

// Presets
const uint8_t DAIKIN_BRC_PRESET_ECO = 0x90;
const uint8_t DAIKIN_BRC_PRESET_BOOST = 0x30;
const uint8_t DAIKIN_BRC_PRESET_COMFORT = 0x00;

// IR Transmission
const uint32_t DAIKIN_BRC_IR_FREQUENCY = 38000;
const uint32_t DAIKIN_BRC_HEADER_MARK = 9921;
const uint32_t DAIKIN_BRC_HEADER_SPACE = 9710;
const uint32_t DAIKIN_BRC_LEAD_IN_MARK = 4658;
const uint32_t DAIKIN_BRC_LEAD_IN_SPACE = 2500;
const uint32_t DAIKIN_BRC_BIT_MARK = 395;
const uint32_t DAIKIN_BRC_ONE_SPACE = 921;
const uint32_t DAIKIN_BRC_ZERO_SPACE = 342;
const uint32_t DAIKIN_BRC_MESSAGE_SPACE = 20163;

const uint8_t DAIKIN_BRC_IR_DRY_FAN_TEMP_F = 72;            // Dry/Fan mode is always 17 Celsius.
const uint8_t DAIKIN_BRC_IR_DRY_FAN_TEMP_C = (17 - 9) * 2;  // Dry/Fan mode is always 17 Celsius.
const uint8_t DAIKIN_BRC_IR_MODE_BUTTON = 0x4;  // This is set after a mode action

//BYTE7
const uint8_t DAIKIN_BRC_IR_SLEEP_ON = 0x06;
const uint8_t DAIKIN_BRC_IR_SLEEP_OFF = 0x04;
const uint8_t DAIKIN_BRC_IR_POWER_TOGGLE_ON = 0x0c;
const uint8_t DAIKIN_BRC_IR_POWER_TOGGLE_OFF = 0x04;
const uint8_t DAIKIN_BRC_IR_SWING_ON = 0x05;
const uint8_t DAIKIN_BRC_IR_SWING_OFF = 0x04;


// State Frame size
const uint8_t DAIKIN_BRC_STATE_FRAME_SIZE = 8;
// Preamble size
const uint8_t DAIKIN_BRC_PREAMBLE_SIZE = 7;

class DaikinBrcClimate : public climate_ir::ClimateIR {
 public:
  DaikinBrcClimate()
      : climate_ir::ClimateIR(DAIKIN_BRC_TEMP_MIN_C, DAIKIN_BRC_TEMP_MAX_C, 0.5f, true, true,
                              {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH},
                              {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_BOTH},
                              {climate::CLIMATE_PRESET_ECO, climate::CLIMATE_PRESET_BOOST, climate::CLIMATE_PRESET_COMFORT}) {}

  /// Set use of Fahrenheit units
  void set_fahrenheit(bool value) {
    this->fahrenheit_ = value;
    this->temperature_step_ = value ? 0.5f : 1.0f;
  }

 protected:
  uint8_t mode_button_ = 0x00;
  // Capture if the MODE was changed
  void control(const climate::ClimateCall &call) override;
  // Transmit via IR the state of this climate controller.
  void setup() override;
  void transmit_state() override;
  uint8_t alt_mode_();
  uint8_t operation_mode_();
  uint8_t fan_speed_();
  uint8_t temperature_();
  uint8_t preset_mode_();
  uint8_t swing_mode_();
  uint8_t sleep_mode_();
  // Handle received IR Buffer
  bool on_receive(remote_base::RemoteReceiveData data) override;
  bool parse_state_frame_(const uint8_t frame[]);
  bool fahrenheit_{false};
  uint8_t last_mode = climate::CLIMATE_MODE_OFF;
};

}  // namespace daikin_brc
}  // namespace esphome
