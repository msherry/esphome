#include "pid_controller.h"
#include "esphome/core/log.h"

namespace esphome {
    namespace pid_controller {

        static const char *TAG = "pid_controller";

        PIDController::PIDController(float kp, float ki, float kd, float out_min, float out_max)
            : kp_(kp), ki_(ki), kd_(kd), out_min_(out_min), out_max_(out_max) {
            // Ki will be handling most of our speed setting in the steady
            // state, so let the integral term rise to a good fraction of max
            INTEGRAL_LIMIT = ki ? 0.8 * out_max / ki_ : 0.0;

            reset();
        }

        void PIDController::setup() {
            ESP_LOGI(TAG, "PID Controller initialized (Kp=%.3f, Ki=%.3f, Kd=%.3f)", kp_, ki_, kd_);
        }

        void PIDController::reset() {
            // Don't start at a standstill
            integral_ = ki_ ? 3.5 / ki_ : 0.0;
            prev_error_ = 0;
            last_time_ = 0;
            last_output_ = 0;

            // Debugging
            last_p_ = 0;
            last_i_ = 0;
            last_d_ = 0;
        }

        float PIDController::update(float setpoint, float measured, uint32_t now_ms) {
            if (last_time_ == 0) {
                last_time_ = now_ms;
                prev_error_ = setpoint - measured;
                return last_output_;
            }

            float dt = (now_ms - last_time_) / 1000.0f;
            last_time_ = now_ms;

            // If it's been too long since our last update, or a nonsensical amount
            // of time, no change.
            if (dt < 0 || dt > 30) {
                return last_output_;
            }

            float error = setpoint - measured;

            // Only integrate if we're close to the setpoint, to prevent too much windup.
            if (std::abs(error) <= 25) {
                integral_ += error * dt;
            }

            // anti-windup clamp
            if (integral_ > INTEGRAL_LIMIT) integral_ = INTEGRAL_LIMIT;
            if (integral_ < -INTEGRAL_LIMIT) integral_ = -INTEGRAL_LIMIT;

            float derivative = (error - prev_error_) / dt;

            float output = kp_ * error + ki_ * integral_ + kd_ * derivative;

            ESP_LOGD(TAG, "proportional: %.0f,  integral: %.0f,  derivative: %.0f,  output: %.1f",
                    error, integral_, derivative, output);

            prev_error_ = error;

            // Clamp output
            if (output > out_max_) output = out_max_;
            if (output < out_min_) output = out_min_;

            last_output_ = output;

            // Debugging
            last_p_ = kp_ * error;
            last_i_ = ki_ * integral_;
            last_d_ = kd_ * derivative;

            return output;
        }
        float PIDController::get_proportional() {return last_p_;}
        float PIDController::get_integral() {return last_i_;}
        float PIDController::get_derivative() {return last_d_;}
    }  // namespace pid_controller
}  // namespace esphome
