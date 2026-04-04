#pragma once

#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"

#include "esphome/components/canbus/canbus.h"

#include <vector>
#include <array>

namespace esphome {
namespace worm_relay {

// CAN ID format (11-bit): prio(1 bit) + devtype(3 bits) + msgtype(3 bits) + addr(4 bits)
// See README.md for protocol details

// Control message: gw -> relay module (set relay states)
static constexpr uint32_t CAN_RELAY_CONTROL_BASE = 0b00000000000;  // prio=0, devtype=000, msgtype=000

// State message: relay module -> gw
static constexpr uint32_t CAN_RELAY_STATE_MSG = 0b00010010000;     // prio=0, devtype=001, msgtype=001
static constexpr uint32_t CAN_RELAY_STATE_MASK = 0b01111110000;    // mask to match state messages

class WormRelayComponent : public Component {
 public:
  WormRelayComponent() = default;

  void setup() override;
  float get_setup_priority() const override;
  void dump_config() override;

  void set_canbus(canbus::Canbus *canbus) { this->canbus_ = canbus; }

  // Get current state for a specific module and pin
  bool get_pin_state(uint8_t addr, uint8_t pin);

  // Write a single relay - defers CAN send; loop() batches and sends per-module
  void set_relay(uint8_t addr, uint8_t relay_no, bool value);

 protected:
  friend class WormRelayGPIOPin;

  void loop() override;

  void send_to_module_(uint8_t addr);
  void on_canbus_frame_(uint32_t can_id, bool extended, bool rtr, const std::vector<uint8_t> &data);

  // State for up to 16 modules, each with 16 relays
  std::array<uint16_t, 16> relay_states_{};

  // Batched sending: tracks which modules have pending state updates
  // In loop(), one CAN frame per dirty module is sent, coalescing multiple relay changes
  uint16_t dirty_modules_{0};
  void mark_dirty_(uint8_t addr);
  
  bool boot_complete_{false};

  canbus::Canbus *canbus_{nullptr};
};

/// Helper class to expose a worm relay virtual output GPIO pin.
class WormRelayGPIOPin : public GPIOPin, public Parented<WormRelayComponent> {
 public:
  void setup() override {}
  void pin_mode(gpio::Flags flags) override {}
  bool digital_read() override;
  void digital_write(bool value) override;
  size_t dump_summary(char *buffer, size_t len) const override;

  void set_addr(uint8_t addr) { addr_ = addr; }  // Module address
  void set_relay_pin(uint8_t relay_pin) { relay_pin_ = relay_pin; }  // Actual relay number (0-15)
  void set_inverted(bool inverted) { inverted_ = inverted; }

  /// Always returns `gpio::Flags::FLAG_OUTPUT`.
  gpio::Flags get_flags() const override { return gpio::Flags::FLAG_OUTPUT; }

 protected:
  uint8_t addr_{0};        // Module address (0-15)
  uint8_t relay_pin_{0};   // Relay number on module (0-15)
  bool inverted_{false};
};

}  // namespace worm_relay
}  // namespace esphome
