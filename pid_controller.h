#pragma once

#include <cstdint>

class PIDControllerX {
public:

    PIDControllerX(float kp, float ki, float kd, float out_min, float out_max)
        : kp_(kp), ki_(ki), kd_(kd), out_min_(out_min), out_max_(out_max) {}

    float update(float setpoint, float measured, uint32_t now_ms)
    {
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
        const float integral_limit = 100;
        if (integral_ > integral_limit) integral_ = integral_limit;
        if (integral_ < -integral_limit) integral_ = -integral_limit;

        float derivative = (error - prev_error_) / dt;

        float output =
            kp_ * error +
            ki_ * integral_ +
            kd_ * derivative;

        prev_error_ = error;

        // Clamp output
        if (output > out_max_) output = out_max_;
        if (output < out_min_) output = out_min_;

        return output;
    }

private:
    float kp_;
    float ki_;
    float kd_;

    float integral_ = 0;
    float prev_error_ = 0;
    float last_output_ = 0;

    float out_min_;
    float out_max_;

    uint32_t last_time_ = 0;

};

PIDControllerX treadmill_pid(
    0.05, // Kp
    0.01, // Ki
    0.02, // Kd
    2.0,  // min speed (mph)
    10.0  // max speed (mph)
);
