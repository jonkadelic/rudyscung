#pragma once

#include "./ecs_components.h"

// Predefines
typedef struct level level_t;

typedef struct ecs ecs_t;

typedef unsigned int entity_t;

typedef void (*ecs_system_t)(ecs_t* const self, level_t* const level, entity_t const entity);

ecs_t* const ecs_new(void);

void ecs_delete(ecs_t* const self);

void ecs_tick(ecs_t* const self, level_t* const level);

entity_t const ecs_new_entity(ecs_t* const self);

void ecs_delete_entity(ecs_t* const self, entity_t const entity);

void* const ecs_attach_component(ecs_t* const self, entity_t const entity, ecs_component_t const component);

void ecs_detach_component(ecs_t* const self, entity_t const entity, ecs_component_t const component);

bool const ecs_has_component(ecs_t const* const self, entity_t const entity, ecs_component_t const component);

void* const ecs_get_component_data(ecs_t* const self, entity_t const entity, ecs_component_t const component);

void ecs_attach_system(ecs_t* const self, ecs_component_t const component, ecs_system_t const system);

void ecs_detach_system(ecs_t* const self, ecs_component_t const component, ecs_system_t const system);

entity_t const ecs_get_highest_entity_id(ecs_t* const self);

bool const ecs_does_entity_exist(ecs_t* const self, entity_t const entity);
