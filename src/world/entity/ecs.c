#include "./ecs.h"

#include <assert.h>
#include <stdlib.h>

#include "ecs_components.h"
#include "../../util/logger.h"

#define MAX_ENTITIES 1024

typedef struct entity_storage {
    void* component_data[NUM_ECS_COMPONENTS];
} entity_storage_t;

typedef struct system_storage {
    ecs_component_t target;
    ecs_system_t system;
} system_storage_t;

struct ecs {
    entity_storage_t* entities[MAX_ENTITIES];
    size_t systems_size;
    system_storage_t** systems;
    entity_t highest_entity_id;
};

static size_t const COMPONENT_SIZES[NUM_ECS_COMPONENTS] = {
    [ECS_COMPONENT__POS] = sizeof(ecs_component_pos_t),
    [ECS_COMPONENT__VEL] = sizeof(ecs_component_vel_t),
    [ECS_COMPONENT__ROT] = sizeof(ecs_component_rot_t),
    [ECS_COMPONENT__AABB] = sizeof(ecs_component_aabb_t),
    [ECS_COMPONENT__GRAVITY] = sizeof(ecs_component_gravity_t),
    [ECS_COMPONENT__SPRITE] = sizeof(ecs_component_sprite_t),
    [ECS_COMPONENT__MOVE_RANDOM] = sizeof(ecs_component_move_random_t)
};

static void* const new_component(ecs_component_t const component);

static void delete_component(ecs_component_t const component, void* const data);

ecs_t* const ecs_new(void) {
    ecs_t* self = calloc(1, sizeof(ecs_t));
    assert(self != nullptr);

    LOG_DEBUG("ecs_t: initialized.");

    return self;
}

void ecs_delete(ecs_t* const self) {
    assert(self != nullptr);

    for (size_t i = 0; i < MAX_ENTITIES; i++) {
        if (self->entities[i] != nullptr) {
            ecs_delete_entity(self, i);
        }
    }

    if (self->systems != nullptr) {
        for (size_t i = 0; i < self->systems_size; i++) {
            free(self->systems[i]);
        }
        free(self->systems);
    }

    free(self);

    LOG_DEBUG("ecs_t: deleted.");
}

void ecs_tick(ecs_t* const self, level_t* const level) {
    assert(self != nullptr);
    assert(level != nullptr);

    for (entity_t entity = 0; entity < MAX_ENTITIES; entity++) {
        if (self->entities[entity] != nullptr) {
            for (size_t i = 0; i < self->systems_size; i++) {
                if (self->systems[i] != nullptr) {
                    system_storage_t const* const system_storage = self->systems[i];
                    if (ecs_has_component(self, entity, system_storage->target)) {
                        system_storage->system(self, level, entity);
                    }
                }
            }
        }
    }
}

entity_t const ecs_new_entity(ecs_t* const self) {
    assert(self != nullptr);

    bool found_slot = false;
    entity_t entity = 0;
    for (entity_t i = 0; i < MAX_ENTITIES; i++) {
        if (self->entities[i] == nullptr) {
            entity = i;
            found_slot = true;
            break;
        }
    }
    assert(found_slot);

    if (self->highest_entity_id < entity) {
        self->highest_entity_id = entity;
    }

    entity_storage_t* entity_storage = calloc(1, sizeof(entity_storage_t));
    assert(entity_storage != nullptr);

    self->entities[entity] = entity_storage;

    return entity;
}

void ecs_delete_entity(ecs_t* const self, entity_t const entity) {
    assert(self != nullptr);
    assert(self->entities[entity] != nullptr);

    entity_storage_t* const entity_storage = self->entities[entity];

    for (ecs_component_t i = 0; i < NUM_ECS_COMPONENTS; i++) {
        if (entity_storage->component_data[i] != nullptr) {
            delete_component(i, entity_storage->component_data[i]);
        }
    }

    free(entity_storage);
    self->entities[entity] = nullptr;
}

void* const ecs_attach_component(ecs_t* const self, entity_t const entity, ecs_component_t const component) {
    assert(self != nullptr);
    assert(self->entities[entity] != nullptr);
    assert(component >= 0 && component < NUM_ECS_COMPONENTS);
    assert(self->entities[entity]->component_data[component] == nullptr);

    size_t const component_storage_size = COMPONENT_SIZES[component];
    assert(component_storage_size > 0);

    entity_storage_t* const entity_storage = self->entities[entity];

    void* const component_storage = new_component(component);

    entity_storage->component_data[component] = component_storage;

    return component_storage;
}

void ecs_detach_component(ecs_t* const self, entity_t const entity, ecs_component_t const component) {
    assert(self != nullptr);
    assert(self->entities[entity] != nullptr);
    assert(component >= 0 && component < NUM_ECS_COMPONENTS);
    assert(self->entities[entity]->component_data[component] != nullptr);

    entity_storage_t* const entity_storage = self->entities[entity];

    delete_component(component, entity_storage->component_data[component]);
    entity_storage->component_data[component] = nullptr;
}

bool const ecs_has_component(ecs_t const* const self, entity_t const entity, ecs_component_t const component) {
    assert(self != nullptr);
    assert(self->entities[entity] != nullptr);
    assert(component >= 0 && component < NUM_ECS_COMPONENTS);

    entity_storage_t* const entity_storage = self->entities[entity];

    return entity_storage->component_data[component] != nullptr;
}

void* const ecs_get_component_data(ecs_t* const self, entity_t const entity, ecs_component_t const component) {
    assert(self != nullptr);
    assert(self->entities[entity] != nullptr);
    assert(component >= 0 && component < NUM_ECS_COMPONENTS);
    assert(self->entities[entity]->component_data[component] != nullptr);

    entity_storage_t* const entity_storage = self->entities[entity];

    return entity_storage->component_data[component];
}

void ecs_attach_system(ecs_t* const self, ecs_component_t const component, ecs_system_t const system) {
    assert(self != nullptr);
    assert(component >= 0 && component < NUM_ECS_COMPONENTS);
    assert(system != nullptr);

    size_t slot = 0;
    bool found_slot = false;
    for (size_t i = 0; i < self->systems_size; i++) {
        if (self->systems[i] == nullptr) {
            slot = i;
            found_slot = true;
            break;
        }
    }

    if (!found_slot) {
        size_t const new_systems_size = self->systems_size + 1;
        self->systems = realloc(self->systems, sizeof(system_storage_t*) * new_systems_size);
        assert(self->systems != nullptr);
        self->systems_size = new_systems_size;
        slot = self->systems_size - 1;
    }

    system_storage_t* const system_storage = calloc(1, sizeof(system_storage_t));
    assert(system_storage != nullptr);

    system_storage->target = component;
    system_storage->system = system;

    self->systems[slot] = system_storage;
}

void ecs_detach_system(ecs_t* const self, ecs_component_t const component, ecs_system_t const system) {
    assert(self != nullptr);
    assert(component >= 0 && component < NUM_ECS_COMPONENTS);
    assert(system != nullptr);

    bool deleted_any = false;
    for (size_t i = 0; i < self->systems_size; i++) {
        if (self->systems[i] != nullptr) {
            system_storage_t* const system_storage = self->systems[i];
            if (component == system_storage->target && system == system_storage->system) {
                free(system_storage);
                self->systems[i] = nullptr;
                deleted_any = true;
            }
        }
    }

    assert(deleted_any);
}

entity_t const ecs_get_highest_entity_id(ecs_t* const self) {
    assert(self != nullptr);

    return self->highest_entity_id;
}

bool const ecs_does_entity_exist(ecs_t* const self, entity_t const entity) {
    assert(self != nullptr);

    if (entity < 0 || entity >= MAX_ENTITIES) {
        return false;
    }

    return self->entities[entity] != nullptr;
}

static void* const new_component(ecs_component_t const component) {
    assert(component >= 0 && component < NUM_ECS_COMPONENTS);

    void* const data = calloc(1, COMPONENT_SIZES[component]);
    assert(data != nullptr);

    switch (component) {
        case ECS_COMPONENT__AABB: {
            ecs_component_aabb_t* c_data = data;
            c_data->aabb = aabb_new((float[NUM_AXES]) { 0.0f, 0.0f, 0.0f }, (float[NUM_AXES]) { 1.0f, 1.0f, 1.0f });
            break;
        }
        case ECS_COMPONENT__GRAVITY: {
            ecs_component_gravity_t* c_data = data;
            c_data->acceleration = 9.8f / 40;
            break;
        }
        default:
            // Do nothing
    }

    return data;
}

static void delete_component(ecs_component_t const component, void* const data) {
    assert(component >= 0 && component < NUM_ECS_COMPONENTS);
    assert(data != nullptr);

    switch (component) {
        case ECS_COMPONENT__AABB: {
            ecs_component_aabb_t* c_data = data;
            aabb_delete(c_data->aabb);
            break;
        }
        default:
            // Do nothing
    }

    free(data);
}