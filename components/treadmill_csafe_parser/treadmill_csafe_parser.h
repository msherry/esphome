#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"

#include <vector>

namespace esphome {
namespace treadmill_csafe_parser {

// CSAFE Protocol Constants
static const uint8_t CSAFE_CMD_GET_STATUS = 0x80;
static const uint8_t CSAFE_CMD_SET_SPEED = 0x25;
static const uint8_t CSAFE_CMD_SET_GRADE = 0x28;
static const uint8_t CSAFE_CMD_GET_SPEED = 0x53;
static const uint8_t CSAFE_CMD_GET_GRADE = 0x58;

// CSAFE Packet Structure
static const uint8_t CSAFE_PACKET_START = 0xF1;
static const uint8_t CSAFE_PACKET_END = 0xF2;
static const uint8_t CSAFE_PACKET_STUFF = 0xF3;

// Packet states for state machine
static const uint8_t PACKET_STATE_IDLE = 0;       // Waiting for start byte
static const uint8_t PACKET_STATE_RECEIVING = 1; // Receiving packet data
static const uint8_t PACKET_STATE_STUFFED = 2;    // Just received stuffing byte

// Units for commands
// Note: CSAFE speed uses 1/10th mph (0.1 mph steps)
// Note: CSAFE grade uses 0.1% grade (not 1%)
static const uint8_t CSAFE_UNIT_MPH = 0x10;          // 1 MPH
static const uint8_t CSAFE_UNIT_TENTH_MPH = 0x11;    // 1/10th mph (0.1 mph steps)
static const uint8_t CSAFE_UNIT_PCT_GRADE = 0x4A;    // 1% grade
static const uint8_t CSAFE_UNIT_TENTH_PCT_GRADE = 0x4C; // 0.1% grade

class TreadmillCSAFEParser : public Component, public uart::UARTDevice {
 public:
  TreadmillCSAFEParser(uart::UARTComponent *parent);

  void setup() override;
  void loop() override;

  void set_speed(float speed_mph);
  void set_incline(float incline_percent);

  void get_speed();
  void get_incline();
  void get_status();

  float get_last_speed() const { return last_speed_; }
  float get_last_incline() const { return last_incline_; }

 protected:
  // UART receive state
  std::vector<uint8_t> packet_buffer_;
  uint8_t packet_state_ = PACKET_STATE_IDLE;
  uint8_t checksum_ = 0;

  float last_speed_ = 0.0f;
  float last_incline_ = 0.0f;

  // Internal methods
  void send_stuffed_byte(uint8_t byte);
  void send_command(uint8_t cmd, uint8_t data_len, const uint8_t *data);
  bool validate_checksum();
  void process_packet();
};

}  // namespace treadmill_csafe_parser
}  // namespace esphome
