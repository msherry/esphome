#ifndef RS232_TEST_HARNESS_H
#define RS232_TEST_HARNESS_H

#include "esphome.h"
#include <vector>

namespace esphome {
namespace rs232 {

class RS232TestHarness : public Component {
 public:
  explicit RS232TestHarness(UARTComponent *parent) : uart_(parent) {}

  void setup() override {
    ESP_LOGD("rs232", "RS232 Test Harness initialized");
    instance_ = this;
  }

  void loop() override {
    uint32_t now = millis();

    // Parse incoming data from UART
    while (uart_->available()) {
      uint8_t byte;
      size_t len = uart_->read_array(&byte, 1);
      if (len == 0) continue;

      handle_received_byte(byte);
      last_rx_time_ = now;  // Track when we last saw a byte
    }

    // If we have data and it's been 50ms since the last byte, publish it
    if (!rx_buffer_.empty() && (now - last_rx_time_ > 50)) {
      publish_response();
    }
  }

  void send_command(const std::vector<uint8_t>& bytes) {
    ESP_LOGD("rs232", "Sending command: %zu bytes", bytes.size());
    for (size_t i = 0; i < bytes.size(); i++) {
      ESP_LOGD("rs232", "  [%zu]: 0x%02X", i, bytes[i]);
    }
    uart_->write_array(bytes);
  }

  void set_response_sensor(text_sensor::TextSensor* sensor) {
    rs232_response_ = sensor;
  }

  void set_data_available_sensor(binary_sensor::BinarySensor* sensor) {
    rs232_data_available_ = sensor;
  }

  static RS232TestHarness *get_instance() { return instance_; }

 private:
  UARTComponent *uart_;
  text_sensor::TextSensor* rs232_response_{nullptr};
  binary_sensor::BinarySensor* rs232_data_available_{nullptr};
  std::vector<uint8_t> rx_buffer_;
  uint32_t last_rx_time_{0};
  static RS232TestHarness *instance_;

  void handle_received_byte(uint8_t byte) {
    rx_buffer_.push_back(byte);

    // Log each byte as it comes in
    ESP_LOGV("rs232", "RX byte: 0x%02X", byte);
  }

  void publish_response() {
    if (rx_buffer_.empty()) return;

    // Convert to hex string for display
    std::string hex_str;
    for (uint8_t byte : rx_buffer_) {
      char buf[4];
      snprintf(buf, sizeof(buf), "%02X ", byte);
      hex_str += buf;
    }

    // Log to serial
    ESP_LOGI("rs232", "Received %zu bytes: %s", rx_buffer_.size(), hex_str.c_str());

    // Update text sensor
    if (rs232_response_ != nullptr) {
      rs232_response_->publish_state(hex_str);
    }
    if (rs232_data_available_ != nullptr) {
      rs232_data_available_->publish_state(true);
    }

    // Clear buffer
    rx_buffer_.clear();
  }
};

RS232TestHarness *RS232TestHarness::instance_ = nullptr;

}  // namespace rs232
}  // namespace esphome

#endif  // RS232_TEST_HARNESS_H