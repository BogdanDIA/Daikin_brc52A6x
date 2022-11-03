#include "daikin_brc1.h"
#include "esphome/components/remote_base/remote_base.h"

namespace esphome {
namespace daikin_brc1 {

static const char *const TAG = "daikin_brc1.climate";

void DaikinBrcClimate::setup() {

  ClimateIR::setup();
}

void DaikinBrcClimate::control(const climate::ClimateCall &call) {
  this->mode_button_ = 0x00;
  if (call.get_mode().has_value()) {
    // Need to determine if this is call due to Mode button pressed so that we can set the Mode button byte
    this->mode_button_ = DAIKIN_BRC_IR_MODE_BUTTON;
  }
  
  // we need this value so that we know what was the last mode of the device
  // we need it when pushing buttons in the front end to see if we toggle power or not
  this->last_mode = this->mode;
  ClimateIR::control(call);
}

void DaikinBrcClimate::transmit_state() {
  uint8_t remote_state[DAIKIN_BRC_STATE_FRAME_SIZE] = {0x16, 0x00, 0x43, 0x02, 0x50, 0x12, 0x00, 0x0c};

  // If no preset BOOST or ECO then the value is for fan speed
  // Else the value contains the preset value
  if (this->preset_mode_() == DAIKIN_BRC_PRESET_COMFORT)
    remote_state[1] = (0xF0 & this->fan_speed_()) | (0x0F & this->operation_mode_());
  else
    remote_state[1] = (0xF0 & this->preset_mode_()) | (0x0F & this->operation_mode_());
  
  // Temperature
  remote_state[6] = this->temperature_();

  // Calculate checksum
  uint8_t checksum = 0;
  for (int i = 0; i < (DAIKIN_BRC_STATE_FRAME_SIZE - 1); i++) {
      checksum += (remote_state[i] & 0x0f) + ((remote_state[i] >> 4) & 0x0f);
  }

  ESP_LOGCONFIG(TAG, "TX: mode_button_ %x", this->mode_button_);
  // calculate the power toggle
  uint8_t power = DAIKIN_BRC_IR_POWER_TOGGLE_OFF;
  if (this->mode_button_ == DAIKIN_BRC_IR_MODE_BUTTON)
  {
    ESP_LOGCONFIG(TAG, "TX: this->last_mode %x", this->last_mode);
    ESP_LOGCONFIG(TAG, "TX: this->mode %x", this->mode);

    if ((this->mode != climate::CLIMATE_MODE_OFF) ^ (this->last_mode != climate::CLIMATE_MODE_OFF))
      power = DAIKIN_BRC_IR_POWER_TOGGLE_ON;
    else
      power = DAIKIN_BRC_IR_POWER_TOGGLE_OFF;
    
    this->last_mode = this->mode;
  }

  // add power, swing, sleep bits to checksum
  power = power | this->swing_mode_() | this->sleep_mode_();
  checksum += power;
  checksum &= 0x0F;
  remote_state[DAIKIN_BRC_STATE_FRAME_SIZE - 1] = (checksum << 4) | power;

  auto transmit = this->transmitter_->transmit();
  auto *data = transmit.get_data();
  data->set_carrier_frequency(DAIKIN_BRC_IR_FREQUENCY);

  data->mark(DAIKIN_BRC_HEADER_MARK);
  data->space(DAIKIN_BRC_HEADER_SPACE);
  data->mark(DAIKIN_BRC_HEADER_MARK);
  data->space(DAIKIN_BRC_HEADER_SPACE);
  data->mark(DAIKIN_BRC_LEAD_IN_MARK);
  data->space(DAIKIN_BRC_LEAD_IN_SPACE);

  for (int i = 0; i < DAIKIN_BRC_STATE_FRAME_SIZE; i++) {
    for (uint8_t mask = 1; mask > 0; mask <<= 1) {  // iterate through bit mask
      data->mark(DAIKIN_BRC_BIT_MARK);
      bool bit = remote_state[i] & mask;
      data->space(bit ? DAIKIN_BRC_ONE_SPACE : DAIKIN_BRC_ZERO_SPACE);
    }
    ESP_LOGCONFIG(TAG, "TX[%i] %x", i, remote_state[i]);
  }

  data->mark(DAIKIN_BRC_BIT_MARK);
  data->space(DAIKIN_BRC_MESSAGE_SPACE);
  data->mark(DAIKIN_BRC_BIT_MARK);
  data->space(0);

  transmit.perform();
}

uint8_t DaikinBrcClimate::alt_mode_() {
  uint8_t alt_mode = 0x00;
  switch (this->mode) {
    case climate::CLIMATE_MODE_DRY:
      alt_mode = 0x23;
      break;
    case climate::CLIMATE_MODE_FAN_ONLY:
      alt_mode = 0x63;
      break;
    case climate::CLIMATE_MODE_HEAT_COOL:
    case climate::CLIMATE_MODE_COOL:
    case climate::CLIMATE_MODE_HEAT:
    default:
      alt_mode = 0x73;
      break;
  }
  return alt_mode;
}

uint8_t DaikinBrcClimate::operation_mode_() {
  uint8_t operating_mode = DAIKIN_BRC_MODE_OFF;
  switch (this->mode) {
    case climate::CLIMATE_MODE_COOL:
      operating_mode |= DAIKIN_BRC_MODE_COOL;
      break;
    case climate::CLIMATE_MODE_DRY:
      operating_mode |= DAIKIN_BRC_MODE_DRY;
      break;
    case climate::CLIMATE_MODE_HEAT:
      operating_mode |= DAIKIN_BRC_MODE_HEAT;
      break;
    case climate::CLIMATE_MODE_HEAT_COOL:
      operating_mode |= DAIKIN_BRC_MODE_AUTO;
      break;
    case climate::CLIMATE_MODE_FAN_ONLY:
      operating_mode |= DAIKIN_BRC_MODE_FAN;
      break;
    case climate::CLIMATE_MODE_OFF:
    default:
      operating_mode = DAIKIN_BRC_MODE_OFF;
      break;
  }

  return operating_mode;
}

uint8_t DaikinBrcClimate::fan_speed_() {
  uint16_t fan_speed;
  switch (this->fan_mode.value()) {
    case climate::CLIMATE_FAN_LOW:
      fan_speed = DAIKIN_BRC_FAN_1;
      break;
    case climate::CLIMATE_FAN_MEDIUM:
      fan_speed = DAIKIN_BRC_FAN_2;
      break;
    case climate::CLIMATE_FAN_HIGH:
      fan_speed = DAIKIN_BRC_FAN_3;
      break;
    default:
      fan_speed = DAIKIN_BRC_FAN_1;
  }

  return fan_speed;
}

uint8_t DaikinBrcClimate::temperature_() {
  uint8_t temp;
  // Force special temperatures depending on the mode
  switch (this->mode) {
    case climate::CLIMATE_MODE_FAN_ONLY:
    case climate::CLIMATE_MODE_DRY:
      if (this->fahrenheit_) {
        return DAIKIN_BRC_IR_DRY_FAN_TEMP_F;
      }
      return DAIKIN_BRC_IR_DRY_FAN_TEMP_C;
    case climate::CLIMATE_MODE_HEAT_COOL:
    default:
      uint8_t temperature;
      // Temperature in remote is in F
      if (this->fahrenheit_) {
        temperature = (uint8_t) roundf(
            clamp<float>(((this->target_temperature * 1.8) + 32), DAIKIN_BRC_TEMP_MIN_F, DAIKIN_BRC_TEMP_MAX_F));
      } else {
        temp = (uint8_t) roundf(this->target_temperature);
        temperature = (temp/10) * 16 + temp % 10;
      }
      return temperature;
  }
}

uint8_t DaikinBrcClimate::preset_mode_() {
  uint8_t preset = DAIKIN_BRC_PRESET_COMFORT;
   
  switch (this->preset.value()) {
    case climate::CLIMATE_PRESET_ECO:
      preset |= DAIKIN_BRC_PRESET_ECO;
      break;
    case climate::CLIMATE_PRESET_BOOST:
      preset |= DAIKIN_BRC_PRESET_BOOST;
      break;
    case climate::CLIMATE_PRESET_COMFORT:
    default:
      preset |= DAIKIN_BRC_PRESET_COMFORT;
      break;
  }

  return preset;
}

uint8_t DaikinBrcClimate::swing_mode_() {
  uint8_t swing = DAIKIN_BRC_IR_SWING_OFF;
   
  switch (this->swing_mode) {
    case climate::CLIMATE_SWING_BOTH:
      swing = DAIKIN_BRC_IR_SWING_ON;
      break;
    case climate::CLIMATE_SWING_OFF:
    default:
      swing = DAIKIN_BRC_IR_SWING_OFF;
  }

  return swing;
}

uint8_t DaikinBrcClimate::sleep_mode_() {
  uint8_t sleep = DAIKIN_BRC_IR_SLEEP_OFF;

  return sleep;
}

bool DaikinBrcClimate::parse_state_frame_(const uint8_t frame[]) {
  uint8_t checksum = 0;

  // calculate checksum
  for (int i = 0; i < (DAIKIN_BRC_STATE_FRAME_SIZE - 1); i++) {
      checksum += (frame[i] & 0x0f) + ((frame[i] >> 4) & 0x0f);
  }
  checksum += frame[DAIKIN_BRC_STATE_FRAME_SIZE - 1] & 0x0F;
  checksum &= 0x0F;
  uint8_t received_checksum = (frame[DAIKIN_BRC_STATE_FRAME_SIZE - 1] >> 4) & 0x0F;

  // verify checksum
  if (received_checksum != checksum) {
    ESP_LOGCONFIG(TAG, "Bad checksum %x vs %x", checksum, received_checksum);
    return false;
  }
  else
    ESP_LOGCONFIG(TAG, "Good checksum %x", checksum);

  // set mode in frontend
  uint8_t mode = frame[1];
  uint8_t power_mode = frame[7] & DAIKIN_BRC_IR_POWER_TOGGLE_ON;
  // If the RX data comes from the frontend
  // do not toggle the power state as it has been toggled by the TX
  if (this->mode_button_ == DAIKIN_BRC_IR_MODE_BUTTON)
  {
    power_mode = DAIKIN_BRC_IR_POWER_TOGGLE_OFF;
    this->mode_button_ = 0;
  }

  if ((this->mode != climate::CLIMATE_MODE_OFF) ^ (power_mode == DAIKIN_BRC_IR_POWER_TOGGLE_ON))
  {
    switch (mode & 0x0F) {
      case DAIKIN_BRC_MODE_COOL:
        this->mode = climate::CLIMATE_MODE_COOL;
        break;
      case DAIKIN_BRC_MODE_DRY:
        this->mode = climate::CLIMATE_MODE_DRY;
        break;
      case DAIKIN_BRC_MODE_HEAT:
        this->mode = climate::CLIMATE_MODE_HEAT;
        break;
      case DAIKIN_BRC_MODE_AUTO:
        this->mode = climate::CLIMATE_MODE_HEAT_COOL;
        break;
      case DAIKIN_BRC_MODE_FAN:
        this->mode = climate::CLIMATE_MODE_FAN_ONLY;
        break;
    }
  }
  else
  {
    this->mode = climate::CLIMATE_MODE_OFF;
  }

  // set fan mode in frontend
  uint8_t fan_mode = frame[1];
  switch (fan_mode & 0xF0) {
    case DAIKIN_BRC_FAN_1:
      this->fan_mode = climate::CLIMATE_FAN_LOW;
      break;
    case DAIKIN_BRC_FAN_2:
      this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
      break;
    case DAIKIN_BRC_FAN_3:
      this->fan_mode = climate::CLIMATE_FAN_HIGH;
      break;
  }

  // set presets in frontend
  // same nibble is used for FAN and PRESET(ECO, BOOST)
  // If no PRESET received then value represents the FAN speed
  // COMFORT=(no ECO nor BOOST)
  uint8_t preset_mode = frame[1];
  switch (preset_mode & 0xF0) {
    case DAIKIN_BRC_PRESET_ECO:
      this->preset = climate::CLIMATE_PRESET_ECO;
      break;
    case DAIKIN_BRC_PRESET_BOOST:
      this->preset = climate::CLIMATE_PRESET_BOOST;
      break;
    default:
      this->preset = climate::CLIMATE_PRESET_COMFORT;
      break;
  }

  // set temperature in frontend
  uint8_t temperature = frame[6];
  float temperature_c;
  if (this->fahrenheit_) {
    temperature_c = clamp<float>(((temperature - 32) / 1.8), DAIKIN_BRC_TEMP_MIN_C, DAIKIN_BRC_TEMP_MAX_C);
  } else {
    temperature_c = (temperature & 0x0f) + ((temperature >> 4) & 0x0f) * 10;
  }
  this->target_temperature = temperature_c;

  uint8_t swing_mode = frame[7];
  switch (swing_mode & 0xF) {
    case DAIKIN_BRC_IR_SWING_ON:
      this->swing_mode = climate::CLIMATE_SWING_BOTH;
      break;
    case DAIKIN_BRC_IR_SWING_OFF:
      this->swing_mode = climate::CLIMATE_SWING_OFF;
      break;
  }

  // set sleep mode in frontend
  uint8_t sleep_mode = frame[7];
  switch (sleep_mode & 0x0F) {
    case DAIKIN_BRC_IR_SLEEP_ON:
      // TODO update frontend
      break;
    case DAIKIN_BRC_IR_SLEEP_OFF:
      // TODO update frontend
    default:
      break;
  }

  // Check if the transmit state is the same with the received one
  if (this->mode_button_ == DAIKIN_BRC_IR_MODE_BUTTON)
  {  
    if (this->last_mode != this->mode)
      ESP_LOGCONFIG(TAG, "RX and TX state differ: %x, %x", this->last_mode, this->mode);
  }
  
  this->publish_state();
  return true;
}

bool DaikinBrcClimate::on_receive(remote_base::RemoteReceiveData data) {
  uint8_t state_frame[DAIKIN_BRC_STATE_FRAME_SIZE] = {};

  if (!data.expect_item(DAIKIN_BRC_HEADER_MARK, DAIKIN_BRC_HEADER_SPACE)) {
    return false;
  }

  ESP_LOGCONFIG(TAG, "header1");
  if (!data.expect_item(DAIKIN_BRC_HEADER_MARK, DAIKIN_BRC_HEADER_SPACE)) {
    return false;
  }
  ESP_LOGCONFIG(TAG, "header2");
  if (!data.expect_item(DAIKIN_BRC_LEAD_IN_MARK,DAIKIN_BRC_LEAD_IN_SPACE)) {
    return false;
  }
  ESP_LOGCONFIG(TAG, "LEAD_IN");

  for (uint8_t pos = 0; pos < 8; pos++) {
    uint8_t byte = 0;
    for (int8_t bit = 0; bit < 8; bit++) {
      if (data.expect_item(DAIKIN_BRC_BIT_MARK, DAIKIN_BRC_ONE_SPACE)) {
        byte |= 1 << bit;
        //ESP_LOGCONFIG(TAG, "pos:%i bit:%i", pos, bit);
      } else if (!data.expect_item(DAIKIN_BRC_BIT_MARK, DAIKIN_BRC_ZERO_SPACE)) {
          //ESP_LOGCONFIG(TAG, "exit -- pos:%i bit:%i", pos, bit);
          break;
      }
    }
    ESP_LOGCONFIG(TAG, "RX[%i] %x", pos, byte);
    state_frame[pos] = byte;
  }

  if (!data.expect_item(DAIKIN_BRC_BIT_MARK,DAIKIN_BRC_MESSAGE_SPACE))
    return false;
 
  ESP_LOGCONFIG(TAG, "MESSAGE");

  return this->parse_state_frame_(state_frame);
}

}  // namespace daikin_brc
}  // namespace esphome
