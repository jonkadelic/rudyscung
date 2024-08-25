#include "./ecs_systems.h"

#include <assert.h>

#include "ecs.h"
#include "ecs_components.h"

void ecs_system_velocity(ecs_t* const self, level_t* const level, entity_t const entity) {
    assert(self != nullptr);
    assert(level != nullptr);
    assert(ecs_has_component(self, entity, ECS_COMPONENT__POS));
    assert(ecs_has_component(self, entity, ECS_COMPONENT__VEL));

    ecs_component_pos_t* pos = ecs_get_component_data(self, entity, ECS_COMPONENT__POS);
    ecs_component_vel_t* vel = ecs_get_component_data(self, entity, ECS_COMPONENT__VEL);

    pos->ox = pos->x;
    pos->oy = pos->y;
    pos->oz = pos->z;

    pos->x += vel->vx;
    pos->y += vel->vy;
    pos->z += vel->vz;
}

void ecs_system_friction(ecs_t* const self, level_t* const level, entity_t const entity) {
    assert(self != nullptr);
    assert(level != nullptr);
    assert(ecs_has_component(self, entity, ECS_COMPONENT__VEL));

    ecs_component_vel_t* vel = ecs_get_component_data(self, entity, ECS_COMPONENT__VEL);

    vel->vx *= 0.6f;
    vel->vy *= 0.6f;
    vel->vz *= 0.6f;
    if (vel->vx < 0.01f && vel->vx > -0.01f) {
        vel->vx = 0;
    }
    if (vel->vy < 0.01f && vel->vy > -0.01f) {
        vel->vy = 0;
    }
    if (vel->vz < 0.01f && vel->vz > -0.01f) {
        vel->vz = 0;
    }
}