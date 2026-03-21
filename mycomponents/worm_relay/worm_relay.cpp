#include "worm_relay.h"
#include "esphome/core/log.h"

namespace esphome {
namespace worm_relay {

static const char *const TAG = "worm_relay";

void WormRelayComponent::setup() {
  ESP_LOGCONFIG(TAG, "WormRelayComponent:");
  ESP_LOGCONFIG(TAG, "  Managing up to 16 relay modules (addr 0-15)");
  ESP_LOGCONFIG(TAG, "  Each module has 16 relays (pins 0-15)");

  // Register CANbus callback for receiving state from relay modules
  if (this->canbus_ != nullptr) {
    this->canbus_->add_callback([this](uint32_t can_id, bool extended, bool rtr,
                                       const std::vector<uint8_t> &data) {
      this->on_canbus_frame_(can_id, extended, rtr, data);
    });
  }
}

void WormRelayComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "WormRelayComponent:");
  for (uint8_t addr = 0; addr < 16; addr++) {
    if (this->relay_states_[addr] != 0) {
      ESP_LOGCONFIG(TAG, "  Module %u state: %04X", addr, this->relay_states_[addr]);
    }
  }
}

float WormRelayComponent::get_setup_priority() const { return setup_priority::IO; }

void WormRelayComponent::on_canbus_frame_(uint32_t can_id, bool extended, bool rtr,
                                          const std::vector<uint8_t> &data) {
  // Only handle standard CAN IDs (11-bit)
  if (extended) {
    return;
  }

  // Check if this is a relay state message
  if ((can_id & CAN_RELAY_STATE_MASK) != CAN_RELAY_STATE_MSG) {
    return;
  }

  // Extract relay module address (lower 4 bits)
  uint8_t module_addr = can_id & 0x0F;

  ESP_LOGD(TAG, "Received for module %u: id=%04X data=%02X%02X", module_addr, can_id, data[1], data[0]);

  // Validate data size
  if (data.size() != 2) {
    ESP_LOGW(TAG, "Invalid state message size %d from module %u", data.size(), module_addr);
    return;
  }

  // Parse relay states (little-endian: byte0=low8, byte1=high8)
  uint16_t states = data[0] | (static_cast<uint16_t>(data[1]) << 8);

  // Update state
  uint16_t old_states = this->relay_states_[module_addr];
  if (old_states != states) {
    ESP_LOGI(TAG, "Module %u state: %04X -> %04X", module_addr, old_states, states);
    this->relay_states_[module_addr] = states;
  }
}

bool WormRelayComponent::get_pin_state(uint8_t addr, uint8_t pin) {
  if (addr >= 16) {
    ESP_LOGW(TAG, "Address %u out of range (0-15)", addr);
    return false;
  }
  if (pin >= 16) {
    ESP_LOGW(TAG, "Pin %u out of range (0-15)", pin);
    return false;
  }

  return (this->relay_states_[addr] >> pin) & 1;
}

void WormRelayComponent::set_relay(uint8_t addr, uint8_t relay_no, bool value) {
  if (addr >= 16) {
    ESP_LOGW(TAG, "Invalid module address %u (0-15)", addr);
    return;
  }
  if (relay_no >= 16) {
    ESP_LOGW(TAG, "Invalid relay no %u (0-15)", relay_no);
    return;
  }

  uint16_t mask = 1 << relay_no;
  uint16_t current = this->relay_states_[addr];
  bool current_bit = (current & mask) != 0;

  // Only proceed if state actually changed
  if (current_bit == value) {
    return;
  }

  if (value) {
    this->relay_states_[addr] |= mask;
  } else {
    this->relay_states_[addr] &= ~mask;
  }

  this->send_to_module_(addr);
}

void WormRelayComponent::send_to_module_(uint8_t addr) {
  if (this->canbus_ == nullptr) {
    ESP_LOGW(TAG, "CANbus not configured, cannot send to module %u", addr);
    return;
  }

  uint32_t can_id = CAN_RELAY_CONTROL_BASE | addr;
  uint16_t states = this->relay_states_[addr];

  std::vector<uint8_t> data(2);
  data[0] = states & 0xFF;
  data[1] = (states >> 8) & 0xFF;

  // Send with standard CAN ID (11-bit), priority=1 (lower priority for reliable delivery)
  this->canbus_->send_data(can_id, false, data);

  ESP_LOGD(TAG, "Sent to module %u: id=%04X data=%02X%02X", addr, can_id, data[1], data[0]);
}

// WormRelayGPIOPin methods
bool WormRelayGPIOPin::digital_read() {
  bool raw_state = this->parent_->get_pin_state(this->addr_, this->relay_pin_);
  return raw_state != this->inverted_;
}

void WormRelayGPIOPin::digital_write(bool value) {
  this->parent_->set_relay(this->addr_, this->relay_pin_, value != this->inverted_);
}

size_t WormRelayGPIOPin::dump_summary(char *buffer, size_t len) const {
  return buf_append_printf(buffer, len, 0, "WR%u@%u via WormRelay", this->relay_pin_, this->addr_);
}

}  // namespace worm_relay
}  // namespace esphome