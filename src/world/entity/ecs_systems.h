#pragma once

#include "./ecs.h"

void ecs_system_velocity(ecs_t* const self, level_t* const level, entity_t const entity);

void ecs_system_friction(ecs_t* const self, level_t* const level, entity_t const entity);
