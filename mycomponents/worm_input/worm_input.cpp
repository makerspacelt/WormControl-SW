#include "worm_input.h"
#include "esphome/core/log.h"

namespace esphome {
namespace worm_input {

static const char *const TAG = "worm_input";

void WormInputComponent::setup() {
  // Register CANbus callback for receiving state from input modules
  if (this->canbus_ != nullptr) {
    this->canbus_->add_callback([this](uint32_t can_id, bool extended, bool rtr,
                                       const std::vector<uint8_t> &data) {
      this->on_canbus_frame_(can_id, extended, rtr, data);
    });
  }
}

void WormInputComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "WormInputComponent:");
  ESP_LOGCONFIG(TAG, "  Manages up to 16 input modules (canbus addr 0000-1111)");
  ESP_LOGCONFIG(TAG, "  Each module has 32 inputs exposed as gpio pins 1-32");
}

float WormInputComponent::get_setup_priority() const { return setup_priority::HARDWARE; }

void WormInputComponent::on_canbus_frame_(uint32_t can_id, bool extended, bool rtr,
                                          const std::vector<uint8_t> &data) { 
  // Only handle standard CAN IDs (11-bit)
  if (extended) {
    return;
  }

  // Check if this is an input state message
  if ((can_id & CAN_INPUT_STATE_MASK) == CAN_INPUT_STATE_MSG) {
    // Extract input module address (lower 4 bits)
    uint8_t module_addr = can_id & 0x0F;

    ESP_LOGD(TAG, "Received input module %u state: canid=%04X data=%02X%02X%02X%02X", module_addr, can_id, data[3], data[2], data[1], data[0]);

    // Validate data size (32 bits = 4 bytes per module)
    if (data.size() != 4) {
      ESP_LOGW(TAG, "Invalid state message size %d from module %u", data.size(), module_addr);
      return;
    }

    // Reconstruct 32-bit state from little-endian CAN data
    uint32_t new_state = static_cast<uint32_t>(data[0]) |
      (static_cast<uint32_t>(data[1]) << 8) |
      (static_cast<uint32_t>(data[2]) << 16) |
      (static_cast<uint32_t>(data[3]) << 24);

    // Update state if changed
    if (this->input_states_[module_addr] != new_state) {
      this->input_states_[module_addr] = new_state;
      ESP_LOGI(TAG, "Module %u state: %08X", module_addr, new_state);
    }
  }
  // TODO handle monitoring messages
}

bool WormInputComponent::get_pin_state(uint8_t addr, uint8_t pin) {
  if (addr >= 16) {
    ESP_LOGW(TAG, "Address %u out of range (0-15)", addr);
    return false;
  }
  if (pin >= 32) {
    ESP_LOGW(TAG, "Pin %u out of range (0-31 internally, 1-32 in YAML)", pin);
    return false;
  }

  return (this->input_states_[addr] >> pin) & 1;
}

// WormInputGPIOPin methods
bool WormInputGPIOPin::digital_read() {
  bool raw_state = this->parent_->get_pin_state(this->addr_, this->input_pin_);
  return raw_state != this->inverted_;
}

void WormInputGPIOPin::digital_write(bool value) {
  // Input pins cannot be written - log warning but do nothing
  ESP_LOGW(TAG, "Attempted digital_write(%d) on input-only pin %u (module addr %u)", 
           value, this->input_pin_ + 1, this->addr_);
}

size_t WormInputGPIOPin::dump_summary(char *buffer, size_t len) const {
  return buf_append_printf(buffer, len, 0, "WI%u@%u via WormInput", this->input_pin_ + 1, this->addr_);
}

}  // namespace worm_input
}  // namespace esphome
