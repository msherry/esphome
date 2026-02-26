#include "esphome.h"

using namespace esphome;

// CSAFE Protocol Constants
#define CSAFE_CMD_SET_SPEED 0x01
#define CSAFE_CMD_SET_INCLINE 0x02
#define CSAFE_CMD_SET_TARGET_HEART_RATE 0x03
#define CSAFE_CMD_GET_SPEED 0x04
#define CSAFE_CMD_GET_INCLINE 0x05
#define CSAFE_CMD_GET_HEART_RATE 0x06
#define CSAFE_CMD_START_WORKOUT 0x07
#define CSAFE_CMD_STOP_WORKOUT 0x08
#define CSAFE_CMD_PAUSE_WORKOUT 0x09
#define CSAFE_CMD_RESUME_WORKOUT 0x0A

// CSAFE Packet Structure
#define CSAFE_PACKET_START 0x55
#define CSAFE_PACKET_END 0x56

class CSAFEParser : public Component {
public:
    CSAFEParser(UARTComponent *parent) : uart_(parent) {}

    void set_speed(float speed_mph) {
        // CSAFE command to set speed
        uint8_t cmd[] = {CSAFE_PACKET_START, CSAFE_CMD_SET_SPEED, 0x02,
                         (uint8_t)(speed_mph * 100), (uint8_t)(speed_mph * 100 >> 8),
                         CSAFE_PACKET_END};
        uart_->write_array(cmd, sizeof(cmd));
    }

    void set_incline(float incline_percent) {
        // CSAFE command to set incline
        uint8_t cmd[] = {CSAFE_PACKET_START, CSAFE_CMD_SET_INCLINE, 0x02,
                         (uint8_t)(incline_percent * 100), (uint8_t)(incline_percent * 100 >> 8),
                         CSAFE_PACKET_END};
        uart_->write_array(cmd, sizeof(cmd));
    }

    void set_target_heart_rate(uint16_t target_hr) {
        // CSAFE command to set target heart rate
        uint8_t cmd[] = {CSAFE_PACKET_START, CSAFE_CMD_SET_TARGET_HEART_RATE, 0x02,
                         (uint8_t)(target_hr & 0xFF), (uint8_t)(target_hr >> 8),
                         CSAFE_PACKET_END};
        uart_->write_array(cmd, sizeof(cmd));
    }

    void start_workout() {
        // CSAFE command to start workout
        uint8_t cmd[] = {CSAFE_PACKET_START, CSAFE_CMD_START_WORKOUT, 0x00, CSAFE_PACKET_END};
        uart_->write_array(cmd, sizeof(cmd));
    }

    void stop_workout() {
        // CSAFE command to stop workout
        uint8_t cmd[] = {CSAFE_PACKET_START, CSAFE_CMD_STOP_WORKOUT, 0x00, CSAFE_PACKET_END};
        uart_->write_array(cmd, sizeof(cmd));
    }

    void pause_workout() {
        // CSAFE command to pause workout
        uint8_t cmd[] = {CSAFE_PACKET_START, CSAFE_CMD_PAUSE_WORKOUT, 0x00, CSAFE_PACKET_END};
        uart_->write_array(cmd, sizeof(cmd));
    }

    void resume_workout() {
        // CSAFE command to resume workout
        uint8_t cmd[] = {CSAFE_PACKET_START, CSAFE_CMD_RESUME_WORKOUT, 0x00, CSAFE_PACKET_END};
        uart_->write_array(cmd, sizeof(cmd));
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
            } else if (byte == CSAFE_PACKET_END) {
                // End of packet - process complete packet
                packet_buffer_.push_back(byte);
                process_packet();
            } else {
                // Data byte
                packet_buffer_.push_back(byte);
            }
        }
    }

private:
    UARTComponent *uart_;
    std::vector<uint8_t> packet_buffer_;

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
            // Handle speed response
            if (length >= 2) {
                float speed = (packet_buffer_[4] | (packet_buffer_[5] << 8)) / 100.0f;
                ESP_LOGD("csafe", "Received speed: %.2f mph", speed);
            }
            break;

        case CSAFE_CMD_GET_INCLINE:
            // Handle incline response
            if (length >= 2) {
                float incline = (packet_buffer_[4] | (packet_buffer_[5] << 8)) / 100.0f;
                ESP_LOGD("csafe", "Received incline: %.2f%%", incline);
            }
            break;

        case CSAFE_CMD_GET_HEART_RATE:
            // Handle heart rate response
            if (length >= 1) {
                uint16_t hr = packet_buffer_[4];
                ESP_LOGD("csafe", "Received heart rate: %d BPM", hr);
            }
            break;

        default:
            ESP_LOGD("csafe", "Received unknown CSAFE command: 0x%02X", command);
            break;
        }
    }
};
