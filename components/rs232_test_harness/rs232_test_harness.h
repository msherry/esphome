#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace rs232_test_harness {

class RS232TestHarness : public Component, public uart::UARTDevice {
 public:
  RS232TestHarness(uart::UARTComponent *parent);

  void setup() override;
  void loop() override;

  void set_response_sensor(text_sensor::TextSensor *sensor);
  void set_data_available_sensor(binary_sensor::BinarySensor *sensor);

  void send_command(const std::vector<uint8_t> &data);

 protected:
  text_sensor::TextSensor *response_sensor_{nullptr};
  binary_sensor::BinarySensor *data_available_sensor_{nullptr};

  std::vector<uint8_t> rx_buffer_;
  uint32_t last_rx_time_{0};

  void publish_response_();
};

}  // namespace rs232_test_harness
}  // namespace esphome
