#include "esphome.h"

using namespace esphome;

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

class CSAFEParser : public Component {
public:
    CSAFEParser(UARTComponent *parent) : uart_(parent) {}

    void get_status() {
        send_command(CSAFE_CMD_GET_STATUS, 0, nullptr);
    }

    void set_speed(float speed_mph) {
        uint16_t speed_raw = (uint16_t)(speed_mph * 100);
        uint8_t data[] = {(uint8_t)(speed_raw & 0xFF), (uint8_t)(speed_raw >> 8), 0x10};
        send_command(CSAFE_CMD_SET_SPEED, 3, data);
    }

    void set_incline(float incline_percent) {
        uint16_t incline_raw = (uint16_t)(incline_percent * 100);
        uint8_t data[] = {(uint8_t)(incline_raw & 0xFF), (uint8_t)(incline_raw >> 8), 0x6A};
        send_command(CSAFE_CMD_SET_GRADE, 3, data);
    }

    void get_speed() {
        send_command(CSAFE_CMD_GET_SPEED, 0, nullptr);
    }

    void get_incline() {
        send_command(CSAFE_CMD_GET_GRADE, 0, nullptr);
    }

    void setup() override {
        ESP_LOGD("csafe", "CSAFE Parser initialized");
    }

    void loop() override {
        // Parse incoming data from UART
        while (uart_->available()) {
            uint8_t byte = uart_->read();

            // Process incoming byte
            if (byte == CSAFE_PACKET_START) {
                // Start of packet - reset buffer
                packet_buffer_.clear();
                packet_buffer_.push_back(byte);
                unstuff_state_ = 0;  // Reset unstuff state
            } else if (byte == CSAFE_PACKET_END) {
                // End of packet - process complete packet
                packet_buffer_.push_back(byte);
                process_packet();
            } else if (byte == CSAFE_PACKET_STUFF) {
                // Next byte is a stuffed byte
                unstuff_state_ = 1;
            } else if (unstuff_state_ == 1) {
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
                    original_byte = byte;  // Shouldn't happen
                }
                packet_buffer_.push_back(original_byte);
                unstuff_state_ = 0;
            } else {
                // Data byte
                packet_buffer_.push_back(byte);
            }
        }
    }

private:
    UARTComponent *uart_;
    std::vector<uint8_t> packet_buffer_;
    uint8_t unstuff_state_ = 0;  // 0 = normal, 1 = expecting stuffed byte

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
            uart_->write(byte);
        }
    }

    void send_command(uint8_t cmd, uint8_t data_len, const uint8_t *data) {
        // Build and send a CSAFE command packet with byte-stuffing
        // Format: START | CMD | LEN | DATA... | CHECKSUM | END
        // Only the command, length, data, and checksum bytes are stuffed
        // Start and end markers are always sent as raw bytes
        uint8_t checksum = 0;

        uart_->write(CSAFE_PACKET_START);

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

        uart_->write(CSAFE_PACKET_END);
    }

    void process_packet() {
        // Simple packet validation
        if (packet_buffer_.size() < 3) {
            return; // Invalid packet
        }

        // Check if packet starts and ends correctly
        if (packet_buffer_[0] != CSAFE_PACKET_START ||
                packet_buffer_[packet_buffer_.size() - 1] != CSAFE_PACKET_END) {
            return; // Invalid packet structure
        }

        // Process the packet based on command
        uint8_t command = packet_buffer_[1];
        uint8_t length = packet_buffer_[2];

        // Validate length
        if (packet_buffer_.size() < 4 + length) {
            return; // Incomplete packet
        }

        switch (command) {
        case CSAFE_CMD_GET_SPEED:
            // Handle speed response (3 data bytes: LSB, MSB, unit)
            if (length >= 3) {
                uint16_t speed_raw = packet_buffer_[4] | (packet_buffer_[5] << 8);
                float speed = speed_raw / 100.0f;
                ESP_LOGD("csafe", "Received speed: %.2f mph", speed);
            }
            break;

        case CSAFE_CMD_GET_GRADE:
            // Handle grade/incline response (3 data bytes: LSB, MSB, unit)
            if (length >= 3) {
                uint16_t grade_raw = packet_buffer_[4] | (packet_buffer_[5] << 8);
                float grade = grade_raw / 100.0f;
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
};
