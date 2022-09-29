#pragma once
#include "flecs.h"

static inline float safe_dt(ecs_iter_t *it) {
    return zpl_min(it->delta_time, 0.03334f);
}

void SystemsImport(ecs_world_t *ecs);