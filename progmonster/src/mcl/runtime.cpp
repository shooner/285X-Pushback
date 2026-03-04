#include "main.h"
#include "mcl/runtime.hpp"

#include <cmath>
#include <vector>

namespace mcl {

Filter<DEFAULT_PARTICLE_COUNT> global_filter;
Position latest_estimate{};
bool estimate_valid = false;

namespace {
Position last_odom_pos{};
bool has_last_odom = false;

std::vector<Reading> collect_readings_stub() {
    return {};
}
}

void initialize_filter_from_chassis(float spread) {
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

    while (opRunning) {
        step_filter_once();
        pros::delay(20);
    }
}

}  // namespace mcl
