#pragma once

#include "mcl/core.hpp"

namespace mcl {

constexpr size_t DEFAULT_PARTICLE_COUNT = 512;

extern Filter<DEFAULT_PARTICLE_COUNT> global_filter;
extern Position latest_estimate;
extern bool estimate_valid;

void initialize_filter_from_chassis(float spread = 3.0f);
void step_filter_once();
void mclRuntime(void* param);

}  // namespace mcl
