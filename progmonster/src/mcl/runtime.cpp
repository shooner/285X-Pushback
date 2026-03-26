#include "main.h"
#include "mcl/runtime.hpp"

#include <cmath>
#include <memory>
#include <vector>

namespace mcl {

Filter<DEFAULT_PARTICLE_COUNT> global_filter;
Position latest_estimate{};
bool estimate_valid = false;
std::size_t active_distance_sensor_count = 0;

namespace {
Position last_odom_pos{};
bool has_last_odom = false;

std::vector<DistanceSensorConfig> sensor_configs = {
    // Configure these entries when hardware is installed.
    // offset_x_in / offset_y_in are in inches from robot center.
    // heading_deg is sensor facing direction in robot frame.
    {6, 0.0f, 5.25f, 0.0f, 8, 2.0f, 110.0f, true},       // Front sensor on port 6: centered (0), 5.25" forward
    {11, -2.5f, 2.25f, 90.0f, 8, 2.0f, 110.0f, true},    // Right sensor on port 11: 2.5" left, 2.25" forward
    {0, 0.0f, 0.0f, 0.0f, 8, 2.0f, 110.0f, false},       // Unused
};

std::vector<std::unique_ptr<pros::Distance>> distance_sensors;

float mm_to_inches(float mm) {
    return mm * (1.0f / 25.4f);
}

float normalize_deg(float deg) {
    while (deg >= 180.0f) deg -= 360.0f;
    while (deg < -180.0f) deg += 360.0f;
    return deg;
}

Point rotate_robot_to_field(Point p, const Rotation& robot_theta) {
    float c = robot_theta.cos();
    float s = robot_theta.sin();
    return Point{p.x * c - p.y * s, p.x * s + p.y * c};
}

std::vector<Reading> collect_readings_stub() {
    std::vector<Reading> readings;
    Position robot_pose = fromLemPose(chassis.getPose());

    if (distance_sensors.empty()) {
        return readings;
    }

    for (std::size_t i = 0; i < sensor_configs.size() && i < distance_sensors.size(); i++) {
        const auto& cfg = sensor_configs[i];
        if (!cfg.enabled || !distance_sensors[i]) {
            continue;
        }

        std::int32_t raw_mm = distance_sensors[i]->get_distance();
        if (raw_mm == PROS_ERR || raw_mm >= 9999 || raw_mm <= 0) {
            continue;
        }

        std::int32_t confidence = distance_sensors[i]->get_confidence();
        if (confidence != PROS_ERR && confidence < cfg.min_confidence) {
            continue;
        }

        float d_in = mm_to_inches(static_cast<float>(raw_mm));
        if (d_in < cfg.min_distance_in || d_in > cfg.max_distance_in) {
            continue;
        }

        Point sensor_offset_robot{cfg.offset_x_in, cfg.offset_y_in};
        Point relative_pos = rotate_robot_to_field(sensor_offset_robot, robot_pose.theta);

        float sensor_heading_deg = normalize_deg(robot_pose.theta.degrees() + cfg.heading_deg);
        Rotation sensor_heading = Rotation::fromDegrees(sensor_heading_deg);
        Point ray_dir{sensor_heading.cos(), sensor_heading.sin()};
        Point proj_relative = relative_pos + ray_dir;

        float bound = d_in < 7.874015f ? 0.590551f : 0.05f * d_in;
        constexpr float kSigmaBound = 3.0f;
        float std_dev = std::max(bound / kSigmaBound, 1e-6f);

        readings.emplace_back(d_in, std_dev, relative_pos, proj_relative);
    }

    return readings;
}
}

std::vector<DistanceSensorConfig>& distance_sensor_configs() {
    return sensor_configs;
}

void configure_distance_sensors() {
    distance_sensors.clear();
    distance_sensors.reserve(sensor_configs.size());
    active_distance_sensor_count = 0;

    for (const auto& cfg : sensor_configs) {
        if (cfg.enabled && cfg.port >= 1 && cfg.port <= 21) {
            distance_sensors.emplace_back(std::make_unique<pros::Distance>(cfg.port));
            active_distance_sensor_count++;
        } else {
            distance_sensors.emplace_back(nullptr);
        }
    }
}

void initialize_filter_from_chassis(float spread) {
    if (distance_sensors.empty()) {
        configure_distance_sensors();
    }

    auto pose = fromLemPose(chassis.getPose());
    global_filter.init(pose.point.x, pose.point.y, spread);
    latest_estimate = pose;
    estimate_valid = true;
    last_odom_pos = pose;
    has_last_odom = true;
}

void step_filter_once() {
    Position current_pose = fromLemPose(chassis.getPose());

    if (!has_last_odom) {
        initialize_filter_from_chassis(3.0f);
        return;
    }

    float dx = current_pose.point.x - last_odom_pos.point.x;
    float dy = current_pose.point.y - last_odom_pos.point.y;
    float odom_std_dev = std::hypot(dx, dy) / 4.0f;

    global_filter.predict(dx, dy, odom_std_dev);

    auto readings = collect_readings_stub();
    global_filter.update(readings);

    Point estimate_xy = global_filter.estimate();
    latest_estimate = Position{estimate_xy, current_pose.theta};
    estimate_valid = true;

    global_filter.resample();
    last_odom_pos = current_pose;
}

void mclRuntime(void* param) {
    (void)param;

    if (!has_last_odom) {
        initialize_filter_from_chassis(3.0f);
    }

    while (opRunning || autonRunning) {
        step_filter_once();
        pros::delay(20);
    }
}

}  // namespace mcl
