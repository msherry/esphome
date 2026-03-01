#include "pid_controller.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pid_controller {

static const char *TAG = "pid_controller";

const float PIDController::INTEGRAL_LIMIT = 100.0f;

PIDController::PIDController(float kp, float ki, float kd, float out_min, float out_max)
    : kp_(kp), ki_(ki), kd_(kd), out_min_(out_min), out_max_(out_max) {}

void PIDController::setup() {
  ESP_LOGI(TAG, "PID Controller initialized (Kp=%.3f, Ki=%.3f, Kd=%.3f)", kp_, ki_, kd_);
}

float PIDController::update(float setpoint, float measured, uint32_t now_ms) {
  if (last_time_ == 0) {
    last_time_ = now_ms;
    prev_error_ = setpoint - measured;
    return last_output_;
  }

  float dt = (now_ms - last_time_) / 1000.0f;

  // If it's been too long since our last update, or a nonsensical amount
  // of time, no change.
  if (dt < 0 || dt > 20) {
    return last_output_;
  }

  last_time_ = now_ms;

  float error = setpoint - measured;

  integral_ += error * dt;

  // anti-windup clamp
  if (integral_ > INTEGRAL_LIMIT) integral_ = INTEGRAL_LIMIT;
  if (integral_ < -INTEGRAL_LIMIT) integral_ = -INTEGRAL_LIMIT;

  float derivative = (error - prev_error_) / dt;

  float output = kp_ * error + ki_ * integral_ + kd_ * derivative;

  prev_error_ = error;

  // Clamp output
  if (output > out_max_) output = out_max_;
  if (output < out_min_) output = out_min_;

  last_output_ = output;
  return output;
}

}  // namespace pid_controller
}  // namespace esphome
