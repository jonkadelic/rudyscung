#pragma once

#include "../../phys/aabb.h"
#include "../../render/sprites.h"

typedef enum ecs_component {
    ECS_COMPONENT__POS,
    ECS_COMPONENT__VEL,
    ECS_COMPONENT__ROT,
    ECS_COMPONENT__AABB,
    ECS_COMPONENT__GRAVITY,
    ECS_COMPONENT__SPRITE,
    NUM_ECS_COMPONENTS
} ecs_component_t;

typedef struct ecs_component_pos {
    float pos[NUM_AXES];
    float pos_o[NUM_AXES];
} ecs_component_pos_t;

typedef struct ecs_component_vel {
    float vel[NUM_AXES];
} ecs_component_vel_t;

typedef struct ecs_component_rot {
    float rot[NUM_ROT_AXES];
} ecs_component_rot_t;

typedef struct ecs_component_aabb {
    aabb_t* aabb;
    bool colliding[NUM_AXES];
} ecs_component_aabb_t;

typedef struct ecs_component_gravity {
    float acceleration;
} ecs_component_gravity_t;

typedef struct ecs_component_sprite {
    sprite_t sprite;
} ecs_component_sprite_t;