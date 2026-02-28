#include "rs232_test_harness.h"
#include "esphome/core/log.h"

namespace esphome {
namespace rs232_test_harness {

static const char *TAG = "rs232_test_harness";

RS232TestHarness::RS232TestHarness(uart::UARTComponent *parent)
    : uart::UARTDevice(parent) {}

void RS232TestHarness::setup() {
  ESP_LOGI(TAG, "RS232 Test Harness initialized");
}

void RS232TestHarness::loop() {
  read_uart_();
  process_packet_timeout_();
  publish_console_();
}

void RS232TestHarness::set_console_sensor(text_sensor::TextSensor *sensor) {
  console_sensor_ = sensor;
}

void RS232TestHarness::set_data_available_sensor(binary_sensor::BinarySensor *sensor) {
  data_available_sensor_ = sensor;
}

void RS232TestHarness::send_command(const std::vector<uint8_t> &data) {

  if (data.empty())
    return;

  ESP_LOGD(TAG, "Sending %u bytes", data.size());

  write_array(data.data(), data.size());
  flush();
}

void RS232TestHarness::read_uart_() {

  uint32_t now = millis();

  while (available()) {

    uint8_t byte;
    read_byte(&byte);

    current_packet_.push_back(byte);

    if (current_packet_.size() >= MAX_PACKET_BYTES) {
      finalize_packet_(" [TRUNCATED]");
    }

    last_rx_time_ = now;
  }
}

void RS232TestHarness::process_packet_timeout_() {

  if (current_packet_.empty())
    return;

  uint32_t now = millis();

  if (now - last_rx_time_ > RX_TIMEOUT_MS) {
    finalize_packet_();
  }
}

void RS232TestHarness::finalize_packet_(const char *suffix) {

  if (current_packet_.empty())
    return;

  std::string line;

  char buf[4];

  for (uint8_t b : current_packet_) {
    snprintf(buf, sizeof(buf), "%02X ", b);
    line += buf;
  }

  line += suffix;

  push_console_line_(line);

  if (data_available_sensor_)
    data_available_sensor_->publish_state(true);

  current_packet_.clear();
}

void RS232TestHarness::push_console_line_(const std::string &line) {

  console_lines_.push_back(line);

  while (console_lines_.size() > MAX_LINES)
    console_lines_.pop_front();
}

void RS232TestHarness::publish_console_() {

  if (!console_sensor_)
    return;

  uint32_t now = millis();

  if (now - last_publish_ < PUBLISH_INTERVAL_MS)
    return;

  last_publish_ = now;

  std::string combined;

  for (auto &line : console_lines_) {
    combined += line;
    combined += "\n";
  }

  console_sensor_->publish_state(combined);
}

}  // namespace rs232_test_harness
}  // namespace esphome
