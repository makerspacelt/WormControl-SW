#pragma once

#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"

#include "esphome/components/canbus/canbus.h"

#include <vector>
#include <array>

namespace esphome {
namespace worm_input {

// CAN ID format (11-bit): prio(1) + devtype(3) + msgtype(3) + addr(4)
// See spec-canbus.md for protocol details

// Input module state message: input module -> gw
// devtype=010, msgtype=001
// Match using mask to catch both priority values
static constexpr uint32_t CAN_INPUT_STATE_MSG = 0b00100010000;   // prio=0, devtype=010, msgtype=001, addr=0
static constexpr uint32_t CAN_INPUT_STATE_MASK = 0b01111110000;  // mask to match state messages

class WormInputComponent : public Component {
 public:
  WormInputComponent() = default;

  void setup() override;
  float get_setup_priority() const override;
  void dump_config() override;

  void set_canbus(canbus::Canbus *canbus) { this->canbus_ = canbus; }

  // Get current state for a specific module and pin
  bool get_pin_state(uint8_t addr, uint8_t pin);

 protected:
  friend class WormInputGPIOPin;

  void on_canbus_frame_(uint32_t can_id, bool extended, bool rtr, const std::vector<uint8_t> &data);

  // State for up to 16 modules, each with 32 inputs
  std::array<uint32_t, 16> input_states_{};

  canbus::Canbus *canbus_{nullptr};
};

/// Helper class to expose a worm input virtual input GPIO pin.
class WormInputGPIOPin : public GPIOPin, public Parented<WormInputComponent> {
 public:
  void setup() override {}
  void pin_mode(gpio::Flags flags) override {}
  bool digital_read() override;
  void digital_write(bool value) override;
  size_t dump_summary(char *buffer, size_t len) const override;

  void set_addr(uint8_t addr) { addr_ = addr; }    // Module address
  void set_input_pin(uint8_t input_pin) { input_pin_ = input_pin; }  // Actual input number (0-31)
  void set_inverted(bool inverted) { inverted_ = inverted; }

  /// Always returns `gpio::Flags::FLAG_INPUT`.
  gpio::Flags get_flags() const override { return gpio::Flags::FLAG_INPUT; }

 protected:
  uint8_t addr_{0};        // Module address (0-15)
  uint8_t input_pin_{0};   // Input number on module (0-31 internally, 1-32 in YAML)
  bool inverted_{false};
};

}  // namespace worm_input
}  // namespace esphome
