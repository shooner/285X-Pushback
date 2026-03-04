#pragma once

#include "api.h"
#include "lemlib/pose.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <optional>
#include <vector>

namespace mcl {

constexpr float FIELD_SIZE = 140.42f;
constexpr float HALF_SIZE = FIELD_SIZE * 0.5f;
constexpr float FIELD_MIN = -HALF_SIZE;
constexpr float FIELD_MAX = HALF_SIZE;

struct Rotation {
    float radians = 0.0f;

    Rotation() = default;
    explicit Rotation(float rad) : radians(rad) {}

    static Rotation fromDegrees(float deg) {
        return Rotation(deg * std::numbers::pi_v<float> / 180.0f);
    }

    float degrees() const {
        return radians * 180.0f / std::numbers::pi_v<float>;
    }

    float cos() const { return std::cos(radians); }
    float sin() const { return std::sin(radians); }
};

struct Point {
    float x = 0.0f;
    float y = 0.0f;

    Point() = default;
    Point(float x, float y) : x(x), y(y) {}

    Point operator+(const Point& rhs) const { return {x + rhs.x, y + rhs.y}; }
    Point operator-(const Point& rhs) const { return {x - rhs.x, y - rhs.y}; }
    Point operator*(float scalar) const { return {x * scalar, y * scalar}; }
};

struct Position {
    Point point;
    Rotation theta;

    Position() = default;
    Position(Point p, Rotation t) : point(p), theta(t) {}
};

inline Position fromLemPose(const lemlib::Pose& pose) {
    return Position{Point{static_cast<float>(pose.x), static_cast<float>(pose.y)}, Rotation::fromDegrees(static_cast<float>(pose.theta))};
}

inline lemlib::Pose toLemPose(const Position& position) {
    return lemlib::Pose(position.point.x, position.point.y, position.theta.degrees());
}

struct XorShift32 {
    uint32_t state;

    explicit XorShift32(uint32_t seed = 0) : state(seed == 0 ? 0x12345678u : seed) {}

    uint32_t next_u32() {
        uint32_t x = state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        state = x;
        return x;
    }

    float next_f32() {
        return (next_u32() >> 8) * (1.0f / (1u << 24));
    }

    float range_f32(float min, float max) {
        return min + (max - min) * next_f32();
    }

    float gaussian(float std_dev) {
        float u1 = std::max(next_f32(), 1e-12f);
        float u2 = next_f32();
        float r = std::sqrt(-2.0f * std::log(u1));
        float angle = 2.0f * std::numbers::pi_v<float> * u2;
        return r * std::cos(angle) * std_dev;
    }
};

struct Line {
    Point start;
    Point end;

    std::optional<float> square_intersect_distance(float center_x, float center_y, float width, float height) const {
        float half_width = width * 0.5f;
        float half_height = height * 0.5f;

        float rel_start_x = start.x - center_x;
        float rel_start_y = start.y - center_y;

        float dx = end.x - start.x;
        float dy = end.y - start.y;

        float best_t = std::numeric_limits<float>::infinity();

        if (std::abs(dx) > 1e-6f) {
            float inv_dx = 1.0f / dx;
            float target_x = dx > 0.0f ? half_width : -half_width;
            float t = (target_x - rel_start_x) * inv_dx;

            if (t >= 0.0f) {
                float y = rel_start_y + t * dy;
                if (std::abs(y) <= half_height) {
                    best_t = t;
                }
            }
        }

        if (std::abs(dy) > 1e-6f) {
            float inv_dy = 1.0f / dy;
            float target_y = dy > 0.0f ? half_height : -half_height;
            float t = (target_y - rel_start_y) * inv_dy;

            if (t >= 0.0f && t < best_t) {
                float x = rel_start_x + t * dx;
                if (std::abs(x) <= half_width) {
                    best_t = t;
                }
            }
        }

        if (best_t < std::numeric_limits<float>::infinity()) {
            return best_t * std::hypot(dx, dy);
        }
        return std::nullopt;
    }
};

struct Reading {
    float recorded;
    float inv_var;
    Point relative_pos;
    Point proj_relative;

    Reading(float recorded, float std_dev, Point relative_pos, Point proj_relative)
        : recorded(recorded),
          inv_var(-0.5f / (std_dev * std_dev)),
          relative_pos(relative_pos),
          proj_relative(proj_relative) {}

    std::optional<float> predict(Point particle_pos) const {
        return Line{relative_pos + particle_pos, proj_relative + particle_pos}
            .square_intersect_distance(0.0f, 0.0f, FIELD_SIZE, FIELD_SIZE);
    }
};

template <size_t N>
struct Filter {
    std::array<float, N> particle_x{};
    std::array<float, N> particle_y{};
    std::array<float, N> particle_weights{};

    std::array<float, N> temp_x{};
    std::array<float, N> temp_y{};
    std::array<float, N> temp_weights{};

    std::array<float, N> presample_x{};
    std::array<float, N> presample_y{};
    std::array<float, N> presample_weights{};

    XorShift32 rng;

    Filter() : rng(static_cast<uint32_t>(pros::micros())) {
        float uniform = 1.0f / static_cast<float>(N);
        particle_weights.fill(uniform);
    }

    void init(float x, float y, float spread) {
        rng = XorShift32(static_cast<uint32_t>(pros::micros()));

        float uniform = 1.0f / static_cast<float>(N);
        for (size_t i = 0; i < N; i++) {
            particle_x[i] = std::clamp(x + rng.range_f32(-spread, spread), FIELD_MIN, FIELD_MAX);
            particle_y[i] = std::clamp(y + rng.range_f32(-spread, spread), FIELD_MIN, FIELD_MAX);
            particle_weights[i] = uniform;
        }
    }

    void predict(float dx, float dy, float std_dev) {
        for (size_t i = 0; i < N; i++) {
            particle_x[i] += dx + rng.gaussian(std_dev);
            particle_y[i] += dy + rng.gaussian(std_dev);
            particle_x[i] = std::clamp(particle_x[i], FIELD_MIN, FIELD_MAX);
            particle_y[i] = std::clamp(particle_y[i], FIELD_MIN, FIELD_MAX);
        }
    }

    void update(const std::vector<Reading>& readings) {
        float max_weight = 0.0f;

        for (size_t i = 0; i < N; i++) {
            float weight = 1.0f;

            for (const auto& reading : readings) {
                auto predicted = reading.predict(Point{particle_x[i], particle_y[i]});
                if (predicted.has_value()) {
                    float error = reading.recorded - *predicted;
                    weight *= std::exp(reading.inv_var * error * error);
                    if (weight == 0.0f) {
                        break;
                    }
                } else {
                    weight = 0.0f;
                    break;
                }
            }

            if (!std::isfinite(weight) || weight < 0.0f) {
                weight = 0.0f;
            }

            particle_weights[i] = weight;
            if (weight > max_weight) {
                max_weight = weight;
            }
        }

        if (max_weight <= 0.0f) {
            float uniform = 1.0f / static_cast<float>(N);
            particle_weights.fill(uniform);
            return;
        }

        float weight_sum = 0.0f;
        for (size_t i = 0; i < N; i++) {
            particle_weights[i] /= max_weight;
            weight_sum += particle_weights[i];
        }

        if (weight_sum <= 0.0f) {
            float uniform = 1.0f / static_cast<float>(N);
            particle_weights.fill(uniform);
            return;
        }

        float inv_weight_sum = 1.0f / weight_sum;
        for (size_t i = 0; i < N; i++) {
            particle_weights[i] *= inv_weight_sum;
        }
    }

    Point estimate() const {
        float est_x = 0.0f;
        float est_y = 0.0f;

        for (size_t i = 0; i < N; i++) {
            est_x += particle_x[i] * particle_weights[i];
            est_y += particle_y[i] * particle_weights[i];
        }

        return Point{est_x, est_y};
    }

    void resample() {
        std::copy(particle_x.begin(), particle_x.end(), presample_x.begin());
        std::copy(particle_y.begin(), particle_y.end(), presample_y.begin());
        std::copy(particle_weights.begin(), particle_weights.end(), presample_weights.begin());

        float inv_n = 1.0f / static_cast<float>(N);
        float offset = rng.next_f32() * inv_n;

        float cumulative_weight = particle_weights[0];
        size_t idx = 0;

        for (size_t i = 0; i < N; i++) {
            float sample = offset + static_cast<float>(i) * inv_n;
            while (sample > cumulative_weight && idx < N - 1) {
                idx++;
                cumulative_weight += particle_weights[idx];
            }

            temp_x[i] = particle_x[idx];
            temp_y[i] = particle_y[idx];
            temp_weights[i] = inv_n;
        }

        std::copy(temp_x.begin(), temp_x.end(), particle_x.begin());
        std::copy(temp_y.begin(), temp_y.end(), particle_y.begin());
        std::copy(temp_weights.begin(), temp_weights.end(), particle_weights.begin());
    }
};

}  // namespace mcl
