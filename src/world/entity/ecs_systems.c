#include "./ecs_systems.h"

#include <assert.h>
#include <float.h>

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

    memcpy(pos->pos_o, pos->pos, sizeof(float) * NUM_AXES);

    if (ecs_has_component(self, entity, ECS_COMPONENT__AABB)) {
        ecs_component_aabb_t* const aabb = ecs_get_component_data(self, entity, ECS_COMPONENT__AABB);

        side_t vel_dir[NUM_AXES];
        for (axis_t a = 0; a < NUM_AXES; a++) {
            if (vel->vel[a] < 0) {
                vel_dir[a] = (side_t) (a * 2);
            } else if (vel->vel[a] > 0) {
                vel_dir[a] = (side_t) (a * 2) + 1;
            } else {
                vel_dir[a] = -1;
            }

            aabb->colliding[a] = false;
        }
        // printf("sides = { %d %d %d }\n", vel_dir[0], vel_dir[1], vel_dir[2]);

        for (side_t side_x = SIDE__NORTH; side_x <= SIDE__SOUTH; side_x++) {
            for (side_t side_y = SIDE__BOTTOM; side_y <= SIDE__TOP; side_y++) {
                for (side_t side_z = SIDE__WEST; side_z <= SIDE__EAST; side_z++) {
                    side_t sides[NUM_AXES] = { side_x, side_y, side_z };

                    if (vel_dir[AXIS__X] == side_x || vel_dir[AXIS__Y] == side_y || vel_dir[AXIS__Z] == side_z) {
                        for (axis_t a = 0; a < NUM_AXES; a++) {
                            if (absf(vel->vel[a]) > 0.0f && vel_dir[a] == sides[a]) {
                                float corner_pos[NUM_AXES];
                                aabb_get_point(aabb->aabb, sides, corner_pos);
                                float real_corner_pos[NUM_AXES] = VEC_ADD_INIT(pos->pos, corner_pos);
                                float d = level_get_distance_on_axis(level, real_corner_pos, sides[a], absf(vel->vel[a]));
                                if (!isnan(d)) {
                                    if (vel->vel[a] < 0) {
                                        d = -d;
                                    }

                                    int offsets[NUM_AXES];
                                    side_get_offsets(sides[a], offsets);

                                    pos->pos[a] += d;
                                    // pos->pos[a] -= offsets[a] * 0.0001f;
                                    vel->vel[a] = 0.0f;
                                    aabb->colliding[a] = true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    for (axis_t a = 0; a < NUM_AXES; a++) {
        pos->pos[a] += vel->vel[a];
    }
}

void ecs_system_friction(ecs_t* const self, level_t* const level, entity_t const entity) {
    assert(self != nullptr);
    assert(level != nullptr);
    assert(ecs_has_component(self, entity, ECS_COMPONENT__VEL));

    ecs_component_vel_t* const vel = ecs_get_component_data(self, entity, ECS_COMPONENT__VEL);

    for (axis_t a = 0; a < NUM_AXES; a++) {
        if (a == AXIS__Y) continue;

        vel->vel[a] *= 0.6f;
        if (vel->vel[a] < 0.01f && vel->vel[a] > -0.01f) {
            vel->vel[a] = 0.0f;
        }
    }
}

void ecs_system_collision(ecs_t* const self, level_t* const level, entity_t const entity) {
    assert(self != nullptr);
    assert(level != nullptr);
    assert(ecs_has_component(self, entity, ECS_COMPONENT__POS));
    assert(ecs_has_component(self, entity, ECS_COMPONENT__VEL));
    assert(ecs_has_component(self, entity, ECS_COMPONENT__AABB));
}

void ecs_system_gravity(ecs_t* const self, level_t* const level, entity_t const entity) {
    assert(self != nullptr);
    assert(level != nullptr);
    assert(ecs_has_component(self, entity, ECS_COMPONENT__GRAVITY));
    assert(ecs_has_component(self, entity, ECS_COMPONENT__VEL));

    ecs_component_gravity_t* const gravity = ecs_get_component_data(self, entity, ECS_COMPONENT__GRAVITY);
    ecs_component_vel_t* const vel = ecs_get_component_data(self, entity, ECS_COMPONENT__VEL);

    vel->vel[AXIS__Y] -= gravity->acceleration;
}