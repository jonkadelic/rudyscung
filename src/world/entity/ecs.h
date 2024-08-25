#pragma once

#include "../level.h"
#include "./ecs_components.h"

typedef struct ecs ecs_t;

typedef unsigned int entity_t;

typedef void (*ecs_system_t)(ecs_t* const self, level_t* const level, entity_t const entity);

ecs_t* const ecs_new(level_t* const level);

void ecs_delete(ecs_t* const self);

void ecs_tick(ecs_t* const self);

entity_t const ecs_new_entity(ecs_t* const self);

void ecs_delete_entity(ecs_t* const self, entity_t const entity);

void* const ecs_attach_component(ecs_t* const self, entity_t const entity, ecs_component_t const component);

void ecs_detach_component(ecs_t* const self, entity_t const entity, ecs_component_t const component);

bool ecs_has_component(ecs_t const* const self, entity_t const entity, ecs_component_t const component);

void* const ecs_get_component_data(ecs_t* const self, entity_t const entity, ecs_component_t const component);

void ecs_attach_system(ecs_t* const self, ecs_component_t const component, ecs_system_t const system);

void ecs_detach_system(ecs_t* const self, ecs_component_t const component, ecs_system_t const system);
