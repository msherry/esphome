#ifndef TREADMILL_CSAFE_PARSER_H
#define TREADMILL_CSAFE_PARSER_H

#include "esphome.h"

// CSAFE Protocol Constants
#define CSAFE_CMD_GET_STATUS 0x80
#define CSAFE_CMD_SET_SPEED 0x25
#define CSAFE_CMD_SET_GRADE 0x28
#define CSAFE_CMD_GET_SPEED 0x53
#define CSAFE_CMD_GET_GRADE 0x58

// CSAFE Packet Structure
#define CSAFE_PACKET_START 0xF1
#define CSAFE_PACKET_END 0xF2
#define CSAFE_PACKET_STUFF 0xF3

// Packet states for state machine
#define PACKET_STATE_IDLE 0        // Waiting for start byte
#define PACKET_STATE_RECEIVING 1   // Receiving packet data
#define PACKET_STATE_STUFFED 2     // Just received stuffing byte

// Units for commands
#define CSAFE_UNIT_MPH 0x10
#define CSAFE_UNIT_POINT_01_MPH 0x11
#define CSAFE_UNIT_PCT_GRADE 0x4A
#define CSAFE_UNIT_POINT_01_PCT_GRADE 0x4C

class CSAFEParser : public Component {
public:
    explicit CSAFEParser(UARTComponent *parent) : uart_(parent) {}

    static CSAFEParser *get_instance() { return instance_; }
    static void set_instance(CSAFEParser *instance) { instance_ = instance; }

    void get_status() {
        send_command(CSAFE_CMD_GET_STATUS, 0, nullptr);
    }

    void set_speed(float speed_mph) {
        uint16_t speed_raw = (uint16_t)(speed_mph * 100);
        uint8_t data[] = {(uint8_t)(speed_raw & 0xFF), (uint8_t)(speed_raw >> 8), CSAFE_UNIT_MPH};
        send_command(CSAFE_CMD_SET_SPEED, 3, data);
    }

    void set_incline(float incline_percent) {
        uint16_t incline_raw = (uint16_t)(incline_percent * 100);
        uint8_t data[] = {(uint8_t)(incline_raw & 0xFF), (uint8_t)(incline_raw >> 8), CSAFE_UNIT_PCT_GRADE};
        send_command(CSAFE_CMD_SET_GRADE, 3, data);
    }

    void get_speed() {
        send_command(CSAFE_CMD_GET_SPEED, 0, nullptr);
    }

    float get_last_speed() const { return last_speed_; }

    void get_incline() {
        send_command(CSAFE_CMD_GET_GRADE, 0, nullptr);
    }

    float get_last_incline() const { return last_incline_; }

    void setup() {
        ESP_LOGD("csafe", "CSAFE Parser initialized");
    }

    void loop() {
        // Parse incoming data from UART
        while (uart_->available()) {
            uint8_t byte;
            size_t len = uart_->read_array(&byte, 1);
            if (len == 0) continue;

            if (byte == CSAFE_PACKET_START) {
                // Start of packet - if we were in the middle of receiving a frame,
                // discard it and resync (missed stop byte from previous frame)
                if (packet_state_ == PACKET_STATE_RECEIVING || packet_state_ == PACKET_STATE_STUFFED) {
                    ESP_LOGD("csafe", "Missed stop byte, discarding incomplete frame");
                }
                packet_state_ = PACKET_STATE_RECEIVING;
                packet_buffer_.clear();
                packet_buffer_.push_back(byte);
                checksum_ = 0;
            } else if (byte == CSAFE_PACKET_END) {
                // End of packet - validate and process
                if (packet_state_ == PACKET_STATE_RECEIVING || packet_state_ == PACKET_STATE_STUFFED) {
                    packet_buffer_.push_back(byte);
                    if (validate_checksum()) {
                        process_packet();
                    } else {
                        ESP_LOGD("csafe", "Checksum validation failed, discarding frame");
                    }
                    packet_state_ = PACKET_STATE_IDLE;
                } else {
                    // Missed start byte, discard and resync
                    packet_state_ = PACKET_STATE_IDLE;
                    ESP_LOGD("csafe", "Missed start byte, discarding frame");
                }
            } else if (byte == CSAFE_PACKET_STUFF) {
                // Next byte is stuffed
                if (packet_state_ == PACKET_STATE_RECEIVING) {
                    packet_state_ = PACKET_STATE_STUFFED;
                }
            } else if (packet_state_ == PACKET_STATE_STUFFED) {
                // This is a stuffed byte value - unstuff it
                uint8_t original_byte;
                if (byte == 0x00) {
                    original_byte = 0xF0;
                } else if (byte == 0x01) {
                    original_byte = CSAFE_PACKET_START;
                } else if (byte == 0x02) {
                    original_byte = CSAFE_PACKET_END;
                } else if (byte == 0x03) {
                    original_byte = CSAFE_PACKET_STUFF;
                } else {
                    original_byte = byte;  // Invalid stuffing, shouldn't happen
                }
                packet_buffer_.push_back(original_byte);
                checksum_ ^= original_byte;
                packet_state_ = PACKET_STATE_RECEIVING;
            } else if (packet_state_ == PACKET_STATE_RECEIVING) {
                // Normal data byte
                packet_buffer_.push_back(byte);
                checksum_ ^= byte;
            } else {
                // In IDLE state - discard stray bytes until we see a start
                ESP_LOGD("csafe", "Discarding byte 0x%02X (waiting for start)", byte);
            }
        }
    }

private:
    UARTComponent *uart_;
    std::vector<uint8_t> packet_buffer_;
    uint8_t packet_state_ = PACKET_STATE_IDLE;  // Current packet state
    uint8_t checksum_ = 0;                      // Running XOR checksum

    void send_stuffed_byte(uint8_t byte) {
        // Send a byte with byte-stuffing if needed
        // Only 0xF0-0xF3 need to be stuffed
        if (byte == CSAFE_PACKET_STUFF) {
            uart_->write_array((uint8_t[]){CSAFE_PACKET_STUFF, 0x03}, 2);
        } else if (byte == 0xF0) {
            uart_->write_array((uint8_t[]){CSAFE_PACKET_STUFF, 0x00}, 2);
        } else if (byte == 0xF1) {
            uart_->write_array((uint8_t[]){CSAFE_PACKET_STUFF, 0x01}, 2);
        } else if (byte == 0xF2) {
            uart_->write_array((uint8_t[]){CSAFE_PACKET_STUFF, 0x02}, 2);
        } else {
            uart_->write_array(&byte, 1);
        }
    }

    void send_command(uint8_t cmd, uint8_t data_len, const uint8_t *data) {
        // Build and send a CSAFE command packet with byte-stuffing
        // Format: START | CMD | LEN | DATA... | CHECKSUM | END
        // Only the command, length, data, and checksum bytes are stuffed
        // Start and end markers are always sent as raw bytes
        uint8_t checksum = 0;

        uint8_t start_byte = CSAFE_PACKET_START;
        uart_->write_array(&start_byte, 1);

        // Send CMD and calculate checksum
        send_stuffed_byte(cmd);
        checksum ^= cmd;

        // Send LEN and calculate checksum
        send_stuffed_byte(data_len);
        checksum ^= data_len;

        // Send DATA and calculate checksum
        for (uint8_t i = 0; i < data_len; i++) {
            send_stuffed_byte(data[i]);
            checksum ^= data[i];
        }

        // Send CHECKSUM
        send_stuffed_byte(checksum);

        uint8_t end_byte = CSAFE_PACKET_END;
        uart_->write_array(&end_byte, 1);
    }

    bool validate_checksum() {
        // The checksum is the XOR of all bytes after unstuffing, excluding START and END.
        // Per spec, the checksum byte itself should XOR to 0 with all other payload bytes.
        // The stored checksum is the last byte before END.
        if (packet_buffer_.size() < 4) {
            return false; // Minimum: START + CMD + LEN + END (no data) is too short
        }

        // The spec says: "checksum is computed with byte-by-byte XORing of the frame contents
        // (e.g., excluding start/stop flags and addresses) to verify frame integrity"
        // This means: checksum = XOR of (CMD, LEN, DATA..., CHECKSUM)
        // So after receiving, XOR of all these should be 0 if valid
        uint8_t frame_checksum = 0;
        for (size_t i = 1; i < packet_buffer_.size() - 1; i++) {
            frame_checksum ^= packet_buffer_[i];
        }

        return frame_checksum == 0;
    }

    void process_packet() {
        // Packet structure after unstuffing: START | CMD | LEN | DATA... | CHECKSUM | END
        if (packet_buffer_.size() < 5) {  // START + CMD + LEN + CHECKSUM + END minimum
            ESP_LOGD("csafe", "Packet too short: %zu bytes", packet_buffer_.size());
            return;
        }

        // Check if packet starts and ends correctly
        if (packet_buffer_[0] != CSAFE_PACKET_START ||
                packet_buffer_[packet_buffer_.size() - 1] != CSAFE_PACKET_END) {
            ESP_LOGD("csafe", "Invalid packet structure");
            return;
        }

        // Process the packet based on command
        uint8_t command = packet_buffer_[1];
        uint8_t length = packet_buffer_[2];

        // Validate length - packet should have: START + CMD + LEN + DATA(length bytes) + CHECKSUM + END
        if (packet_buffer_.size() != 4 + length) {
            ESP_LOGD("csafe", "Length mismatch: header says %d, actual data + checksum = %zu",
                     length, packet_buffer_.size() - 4);
            return;
        }

        switch (command) {
        case CSAFE_CMD_GET_SPEED:
            // Handle speed response (3 data bytes: LSB, MSB, unit)
            if (length >= 3) {
                uint16_t speed_raw = packet_buffer_[4] | (packet_buffer_[5] << 8);
                float speed = speed_raw / 100.0f;
                last_speed_ = speed;
                ESP_LOGD("csafe", "Received speed: %.2f mph", speed);
            }
            break;

        case CSAFE_CMD_GET_GRADE:
            // Handle grade/incline response (3 data bytes: LSB, MSB, unit)
            if (length >= 3) {
                uint16_t grade_raw = packet_buffer_[4] | (packet_buffer_[5] << 8);
                float grade = grade_raw / 100.0f;
                last_incline_ = grade;
                ESP_LOGD("csafe", "Received grade/incline: %.2f%%", grade);
            }
            break;

        case CSAFE_CMD_GET_STATUS:
            // Handle status response
            ESP_LOGD("csafe", "Received status response, length: %d", length);
            break;

        default:
            ESP_LOGD("csafe", "Received unknown CSAFE command: 0x%02X", command);
            break;
        }
    }

    static CSAFEParser *instance_;

    float last_speed_ = 0.0;  // Last parsed speed from treadmill
    float last_incline_ = 0.0;  // Last parsed grade from treadmill
};

CSAFEParser *CSAFEParser::instance_ = nullptr;

// Helper macro to get the global CSAFE parser instance
#define CSAFE_GET_INSTANCE() (CSAFEParser::get_instance())

#endif  // TREADMILL_CSAFE_PARSER_H
