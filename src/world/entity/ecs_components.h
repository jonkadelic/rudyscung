#pragma once

#include "../../phys/aabb.h"

typedef enum ecs_component {
    ECS_COMPONENT__POS,
    ECS_COMPONENT__VEL,
    ECS_COMPONENT__ROT,
    ECS_COMPONENT__AABB,
    NUM_ECS_COMPONENTS
} ecs_component_t;

typedef struct ecs_component_pos {
    float x, y, z;
    float ox, oy, oz;
} ecs_component_pos_t;

typedef struct ecs_component_vel {
    float vx, vy, vz;
} ecs_component_vel_t;

typedef struct ecs_component_rot {
    float y_rot, x_rot;
} ecs_component_rot_t;

typedef struct ecs_component_aabb {
    aabb_t* aabb;
    bool colliding[3];
} ecs_component_aabb_t;