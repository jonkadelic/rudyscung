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
                                float p = level_get_nearest_face_on_axis(level, real_corner_pos, sides[a], absf(vel->vel[a]));
                                if (!isnan(p)) {
                                    float next_real_corner_pos[NUM_AXES] = VEC_ADD_INIT(real_corner_pos, vel->vel);
                                    int offsets[NUM_AXES];
                                    side_get_offsets(sides[a], offsets);
                                    if (offsets[a] > 0) {
                                        p -= 0.00001f;
                                    }

                                    if ((offsets[a] > 0 && next_real_corner_pos[a] > p) || (offsets[a] < 0 && next_real_corner_pos[a] < p)) {
                                        pos->pos[a] = p - corner_pos[a];
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
        if (a == AXIS__Y && ecs_has_component(self, entity, ECS_COMPONENT__GRAVITY)) continue;

        vel->vel[a] *= 0.6f;
        if (vel->vel[a] < 0.01f && vel->vel[a] > -0.01f) {
            vel->vel[a] = 0.0f;
        }
    }
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

void ecs_system_move_random(ecs_t* const self, level_t* const level, entity_t const entity) {
    assert(self != nullptr);
    assert(level != nullptr);
    assert(ecs_has_component(self, entity, ECS_COMPONENT__POS));
    assert(ecs_has_component(self, entity, ECS_COMPONENT__VEL));
    assert(ecs_has_component(self, entity, ECS_COMPONENT__ROT));
    assert(ecs_has_component(self, entity, ECS_COMPONENT__MOVE_RANDOM));

    ecs_component_vel_t* const vel = ecs_get_component_data(self, entity, ECS_COMPONENT__VEL);
    ecs_component_rot_t* const rot = ecs_get_component_data(self, entity, ECS_COMPONENT__ROT);

    random_t* const rand = level_get_random(level);

    if (random_next_int_bounded(rand, 100) == 0) {
        rot->rot[ROT_AXIS__Y] = M_PI * 2 * random_next_float(rand);
    }

    bool jump = false;
    if (random_next_int_bounded(rand, 50) == 0) {
        jump = true;
    }

    float forward = 0.3f;
    float cos_rot_dy = cos(rot->rot[ROT_AXIS__Y]);
    float sin_rot_dy = sin(rot->rot[ROT_AXIS__Y]);

    float vel_x = forward * sin_rot_dy;
    float vel_z = -forward * cos_rot_dy;

    vel->vel[AXIS__X] = vel_x;
    vel->vel[AXIS__Z] = vel_z;

    if (jump) {
        vel->vel[AXIS__Y] = 1.0f;
    }
}