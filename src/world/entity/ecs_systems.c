#include "./ecs_systems.h"

#include <assert.h>

#include "ecs.h"
#include "ecs_components.h"
#include "../level.h"
#include "../../util.h"

void ecs_system_velocity(ecs_t* const self, level_t* const level, entity_t const entity) {
    assert(self != nullptr);
    assert(level != nullptr);
    assert(ecs_has_component(self, entity, ECS_COMPONENT__POS));
    assert(ecs_has_component(self, entity, ECS_COMPONENT__VEL));

    ecs_component_pos_t* const pos = ecs_get_component_data(self, entity, ECS_COMPONENT__POS);
    ecs_component_vel_t* const vel = ecs_get_component_data(self, entity, ECS_COMPONENT__VEL);

    pos->ox = pos->x;
    pos->oy = pos->y;
    pos->oz = pos->z;

    pos->x += vel->vx;
    pos->y += vel->vy;
    pos->z += vel->vz;

    if (ecs_has_component(self, entity, ECS_COMPONENT__AABB)) {
        ecs_component_aabb_t const* const aabb = ecs_get_component_data(self, entity, ECS_COMPONENT__AABB);

        if (aabb->colliding[0]) vel->vx = 0;
        if (aabb->colliding[1]) vel->vy = 0;
        if (aabb->colliding[2]) vel->vz = 0;
    }
}

void ecs_system_friction(ecs_t* const self, level_t* const level, entity_t const entity) {
    assert(self != nullptr);
    assert(level != nullptr);
    assert(ecs_has_component(self, entity, ECS_COMPONENT__VEL));

    ecs_component_vel_t* const vel = ecs_get_component_data(self, entity, ECS_COMPONENT__VEL);

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

void ecs_system_collision(ecs_t* const self, level_t* const level, entity_t const entity) {
    assert(self != nullptr);
    assert(level != nullptr);
    assert(ecs_has_component(self, entity, ECS_COMPONENT__POS));
    assert(ecs_has_component(self, entity, ECS_COMPONENT__VEL));
    assert(ecs_has_component(self, entity, ECS_COMPONENT__AABB));

    ecs_component_pos_t* const pos = ecs_get_component_data(self, entity, ECS_COMPONENT__POS);
    ecs_component_vel_t* const vel = ecs_get_component_data(self, entity, ECS_COMPONENT__VEL);
    ecs_component_aabb_t* const aabb = ecs_get_component_data(self, entity, ECS_COMPONENT__AABB);

    side_t vel_dir[3];
    if (vel->vx < 0) {
        vel_dir[0] = SIDE__NORTH;
    } else if (vel->vx > 0) {
        vel_dir[0] = SIDE__SOUTH;
    } else {
        vel_dir[0] = -1;
    }
    if (vel->vy < 0) {
        vel_dir[1] = SIDE__BOTTOM;
    } else if (vel->vy > 0) {
        vel_dir[1] = SIDE__TOP;
    } else {
        vel_dir[1] = -1;
    }
    if (vel->vz < 0) {
        vel_dir[2] = SIDE__WEST;
    } else if (vel->vz > 0) {
        vel_dir[2] = SIDE__EAST;
    } else {
        vel_dir[2] = -1;
    }

    float vel_arr[3] = { vel->vx, vel->vy, vel->vz };

    aabb->colliding[0] = false;
    aabb->colliding[1] = false;
    aabb->colliding[2] = false;

    for (side_t side_x = SIDE__NORTH; side_x <= SIDE__SOUTH; side_x++) {
        for (side_t side_y = SIDE__BOTTOM; side_y <= SIDE__TOP; side_y++) {
            for (side_t side_z = SIDE__WEST; side_z <= SIDE__EAST; side_z++) {
                side_t sides[3] = { side_x, side_y, side_z };
                if (vel_dir[0] == side_x || vel_dir[1] == side_y || vel_dir[2] == side_z) {
                    for (size_t i = 0; i < 3; i++) {
                        if (vel_dir[i] == sides[i]) {
                            float corner_pos[3];
                            aabb_get_point(aabb->aabb, sides, corner_pos);
                            float d = level_get_distance_on_axis(level, pos->x + corner_pos[0], pos->y + corner_pos[1], pos->z + corner_pos[2], sides[i], 1.0f);
                            if (!isnan(d)) {
                                if (d > 0.01f) {
                                    if (vel_arr[i] < 0) {
                                        d = -d;
                                    }
                                    vel_arr[i] = d;
                                    aabb->colliding[i] = true;
                                } else {
                                    vel_arr[i] = 0.0f;
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }
    }

    vel->vx = vel_arr[0];
    vel->vy = vel_arr[1];
    vel->vz = vel_arr[2];
}