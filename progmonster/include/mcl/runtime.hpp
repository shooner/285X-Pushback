#pragma once

#include "mcl/core.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace mcl {

constexpr size_t DEFAULT_PARTICLE_COUNT = 512;

struct DistanceSensorConfig {
	uint8_t port;
	float offset_x_in;
	float offset_y_in;
	float heading_deg;
	int min_confidence;
	float min_distance_in;
	float max_distance_in;
	bool enabled;
};

extern Filter<DEFAULT_PARTICLE_COUNT> global_filter;
extern Position latest_estimate;
extern bool estimate_valid;
extern std::size_t active_distance_sensor_count;

std::vector<DistanceSensorConfig>& distance_sensor_configs();
void configure_distance_sensors();
void initialize_filter_from_chassis(float spread = 3.0f);
void step_filter_once();
void mclRuntime(void* param);

}  // namespace mcl
