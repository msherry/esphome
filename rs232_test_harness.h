#ifndef RS232_TEST_HARNESS_H
#define RS232_TEST_HARNESS_H

#include "esphome.h"
#include <vector>

namespace esphome {
namespace rs232 {

// Forward declaration
class RS232TestHarness;

// Global instance pointer - declared extern here, defined after class
extern RS232TestHarness *rs232_harness_instance;

class RS232TestHarness {
 public:
  RS232TestHarness() {}

  void setup() {
    ESP_LOGD("rs232", "RS232 Test Harness setup");
    if (uart_ == nullptr) {
      ESP_LOGE("rs232", "UART component is NULL!");
    }
    rs232_harness_instance = this;
  }

  void loop() {
      static uint32_t last_debug = 0;
      uint32_t now = millis();
      if (uart_ == nullptr) {
          ESP_LOGE("rs232", "UART component is NULL in loop()!");
          return;
      }

      // Periodic debug to show component is running
      if (now - last_debug > 1000) {
          ESP_LOGD("rs232_debug", "Component loop running, rx_buffer_ size: %zu", rx_buffer_.size());
          last_debug = now;
      }

    // Check if UART has data available
    int available = uart_->available();
    if (available > 0) {
        ESP_LOGD("rs232_debug", "UART reports %d bytes available", available);

        // Read all available bytes
        while (uart_->available()) {
          uint8_t byte;
          int read_count = uart_->read_array(&byte, 1);
          if (read_count > 0) {
              ESP_LOGI("rs232_raw", "RAW BYTE RECEIVED: 0x%02X ('%c')", byte,
                      (byte >= 32 && byte <= 126) ? byte : '.');
            rx_buffer_.push_back(byte);
            last_rx_time_ = now;
          } else {
            ESP_LOGW("rs232_debug", "read_array returned 0, breaking");
            break;
          }
        }
    }

    if (!rx_buffer_.empty() && (now - last_rx_time_ > 50)) {
      ESP_LOGD("rs232_debug", "Publishing response with %zu bytes", rx_buffer_.size());
      publish_response();
    }
  }


  void send_command(const std::vector<uint8_t>& bytes) {
      if (bytes.empty()) {
          ESP_LOGW("rs232", "Command is empty, skipping send.");
          return;
      }

    if (uart_ == nullptr) {
      ESP_LOGE("rs232", "UART is null, cannot send!");
      return;
    }

    ESP_LOGD("rs232", "Sending %zu bytes", bytes.size());
    for (uint8_t byte : bytes) {
      ESP_LOGD("rs232", "  -> 0x%02X", byte);
    }
    uart_->write_array(bytes);
    uart_->flush(); // Ensure data is actually sent
    ESP_LOGD("rs232", "Send complete");
  }

  void set_uart(UARTComponent *uart) { uart_ = uart; }
  void set_response_sensor(text_sensor::TextSensor* sensor) { rs232_response_ = sensor; }
  void set_data_available_sensor(binary_sensor::BinarySensor* sensor) { rs232_data_available_ = sensor; }

  static RS232TestHarness *get_instance() { return rs232_harness_instance; }

 private:
  UARTComponent *uart_;
  text_sensor::TextSensor* rs232_response_{nullptr};
  binary_sensor::BinarySensor* rs232_data_available_{nullptr};
  std::vector<uint8_t> rx_buffer_;
  uint32_t last_rx_time_{0};

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

// Define the global pointer after class definition
RS232TestHarness *rs232_harness_instance = nullptr;

}  // namespace rs232
}  // namespace esphome
#endif