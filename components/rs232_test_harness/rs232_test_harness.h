#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

#include <vector>
#include <deque>
#include <string>

namespace esphome {
namespace rs232_test_harness {

class RS232TestHarness : public Component, public uart::UARTDevice {
 public:
  RS232TestHarness(uart::UARTComponent *parent);

  void setup() override;
  void loop() override;

  void send_command(const std::vector<uint8_t> &data);

  void set_console_sensor(text_sensor::TextSensor *sensor);
  void set_data_available_sensor(binary_sensor::BinarySensor *sensor);

 protected:
  // UART receive state
  std::vector<uint8_t> current_packet_;
  uint32_t last_rx_time_{0};

  // Console buffer
  std::deque<std::string> console_lines_;
  uint32_t last_publish_{0};

  // Sensors
  text_sensor::TextSensor *console_sensor_{nullptr};
  binary_sensor::BinarySensor *data_available_sensor_{nullptr};

  // Limits
  static const size_t MAX_PACKET_BYTES = 128;
  static const size_t MAX_LINES = 20;
  static const uint32_t RX_TIMEOUT_MS = 20;
  static const uint32_t PUBLISH_INTERVAL_MS = 100;

  // Internal methods
  void read_uart_();
  void process_packet_timeout_();
  void finalize_packet_(const char *suffix = "");
  void push_console_line_(const std::string &line);
  void publish_console_();
};

}  // namespace rs232_test_harness
}  // namespace esphome
