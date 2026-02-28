#include "rs232_test_harness.h"
#include "esphome/core/log.h"

namespace esphome {
namespace rs232_test_harness {

static const char *TAG = "rs232_test_harness";

RS232TestHarness::RS232TestHarness(uart::UARTComponent *parent)
    : uart::UARTDevice(parent) {}

void RS232TestHarness::setup() {
  ESP_LOGD(TAG, "RS232 Test Harness initialized");
}

void RS232TestHarness::loop() {
  uint32_t now = millis();

  while (available()) {
    uint8_t byte;

    if (read_byte(&byte)) {
      ESP_LOGV(TAG, "RX byte: 0x%02X", byte);

      rx_buffer_.push_back(byte);
      last_rx_time_ = now;
    }
  }

  if (!rx_buffer_.empty() && now - last_rx_time_ > 50) {
    publish_response_();
  }
}

void RS232TestHarness::send_command(const std::vector<uint8_t> &data) {
  ESP_LOGD(TAG, "Sending %u bytes", data.size());

  write_array(data);
  flush();
}

void RS232TestHarness::set_response_sensor(text_sensor::TextSensor *sensor) {
  response_sensor_ = sensor;
}

void RS232TestHarness::set_data_available_sensor(binary_sensor::BinarySensor *sensor) {
  data_available_sensor_ = sensor;
}

void RS232TestHarness::publish_response_() {
  std::string hex;

  char buf[4];

  for (auto byte : rx_buffer_) {
    snprintf(buf, sizeof(buf), "%02X ", byte);
    hex += buf;
  }

  ESP_LOGD(TAG, "Received: %s", hex.c_str());

  if (response_sensor_)
    response_sensor_->publish_state(hex);

  if (data_available_sensor_)
    data_available_sensor_->publish_state(true);

  rx_buffer_.clear();
}

}  // namespace rs232_test_harness
}  // namespace esphome
