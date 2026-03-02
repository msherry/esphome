#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"

#include <cstdint>

namespace esphome {
namespace pid_controller {

class PIDController : public Component {
 public:
  PIDController() = default;
  PIDController(float kp, float ki, float kd, float out_min, float out_max);

  void setup() override;

  float update(float setpoint, float measured, uint32_t now_ms);

  void set_kp(float kp) { kp_ = kp; }
  void set_ki(float ki) { ki_ = ki; }
  void set_kd(float kd) { kd_ = kd; }
  void set_output_limits(float out_min, float out_max);

    float get_output_speed();

 protected:
  float kp_;
  float ki_;
  float kd_;

  float integral_ = 0;
  float prev_error_ = 0;
  float last_output_ = 0;

  float out_min_;
  float out_max_;

    float output_speed_ = 0;

  uint32_t last_time_ = 0;

  static const float INTEGRAL_LIMIT;
};

}  // namespace pid_controller
}  // namespace esphome
