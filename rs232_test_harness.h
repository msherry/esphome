#ifndef RS232_TEST_HARNESS_H
#define RS232_TEST_HARNESS_H

#include "esphome.h"
#include <vector>

namespace esphome {
namespace rs232 {

class RS232TestHarness : public Component {
 public:
  // FIX: Pass the UART component in the constructor
  explicit RS232TestHarness(UARTComponent *parent) : uart_(parent) {
    instance_ = this; // FIX: Set the instance here so it's never null
  }

  void setup() override {
    ESP_LOGD("rs232", "RS232 Test Harness initialized");
  }

  // void loop() override {
  //   uint32_t now = millis();
  //   if (uart_ == nullptr) return;

  //   // Temporary: Log whenever the UART thinks it sees data
  //   if (uart_->available() > 0) {
  //       ESP_LOGD("rs232_debug", "UART reports %d bytes available", uart_->available());
  //   }

  //   while (uart_->available()) {
  //     uint8_t byte;
  //     if (uart_->read_array(&byte, 1) > 0) {
  //       rx_buffer_.push_back(byte);
  //       last_rx_time_ = now;
  //     }
  //   }

  //   if (!rx_buffer_.empty() && (now - last_rx_time_ > 50)) {
  //     publish_response();
  //   }
  // }

    void loop() override {
  uint32_t now = millis();
  if (uart_ == nullptr) return;

  // 1. Raw ESP-IDF check: Does the driver think there's data?
  size_t buffered_len = 0;
  uart_get_buffered_data_len(UART_NUM_1, &buffered_len);
  if (buffered_len > 0) {
    ESP_LOGD("rs232_raw", "IDF driver reports %zu bytes waiting", buffered_len);
  }

  // 2. Immediate Read: No buffers, no timers
  uint8_t dummy_byte;
  // We use the underlying IDF call to see if it bypasses the "deafness"
  int rx_len = uart_read_bytes(UART_NUM_1, &dummy_byte, 1, 0);

  if (rx_len > 0) {
    ESP_LOGI("rs232_raw", "RAW BYTE RECEIVED: 0x%02X ('%c')", dummy_byte,
             (dummy_byte >= 32 && dummy_byte <= 126) ? dummy_byte : '.');

    // Still feed the buffer for your UI
    rx_buffer_.push_back(dummy_byte);
    last_rx_time_ = now;
  }

  // 3. Original publishing logic (keep this)
  if (!rx_buffer_.empty() && (now - last_rx_time_ > 50)) {
    publish_response();
  }
}

  void send_command(const std::vector<uint8_t>& bytes) {
      if (bytes.empty()) {
          ESP_LOGW("rs232", "Command is empty, skipping send.");
          return;
      }

    if (uart_ == nullptr) return;

    ESP_LOGD("rs232", "Sending %zu bytes", bytes.size());
    uart_->write_array(bytes);
    uart_->flush(); // Ensure data is actually sent
  }

  void set_response_sensor(text_sensor::TextSensor* sensor) { rs232_response_ = sensor; }
  void set_data_available_sensor(binary_sensor::BinarySensor* sensor) { rs232_data_available_ = sensor; }

  static RS232TestHarness *get_instance() { return instance_; }

 private:
  UARTComponent *uart_;
  text_sensor::TextSensor* rs232_response_{nullptr};
  binary_sensor::BinarySensor* rs232_data_available_{nullptr};
  std::vector<uint8_t> rx_buffer_;
  uint32_t last_rx_time_{0};
  static RS232TestHarness *instance_;

  void publish_response() {
    std::string hex_str;
    for (uint8_t byte : rx_buffer_) {
      char buf[4];
      snprintf(buf, sizeof(buf), "%02X ", byte);
      hex_str += buf;
    }
    if (rs232_response_) rs232_response_->publish_state(hex_str);
    if (rs232_data_available_) rs232_data_available_->publish_state(true);
    rx_buffer_.clear();
  }
};

RS232TestHarness *RS232TestHarness::instance_ = nullptr;

}  // namespace rs232
}  // namespace esphome
#endif
