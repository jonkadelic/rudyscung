#include "./object_counter.h"

#include <stddef.h>
#include <string.h>

#include "src/util/logger.h"

#define ENABLE_OBJECT_COUNTER 1

#define MAX_ENTRIES 1024

typedef struct entry {
    char const* name;
    size_t num_created;
    size_t num_deleted;
} entry_t;

#if ENABLE_OBJECT_COUNTER
static entry_t ENTRIES[MAX_ENTRIES];
#endif

void object_counter_increment(char const* const name) {
#if ENABLE_OBJECT_COUNTER
    for (size_t i = 0; i < MAX_ENTRIES; i++) {
        entry_t* entry = &(ENTRIES[i]);

        if (entry->name == nullptr) {
            entry->name = name;
            entry->num_created = 1;
            return;
        }
        if (entry->name == name) {
            entry->num_created++;
            return;
        }
    }

    LOG_ERROR("object_counter_t: MAX_ENTRIES reached.");
#endif
}

void object_counter_decrement(char const* const name) {
#if ENABLE_OBJECT_COUNTER
    for (size_t i = 0; i < MAX_ENTRIES; i++) {
        entry_t* entry = &(ENTRIES[i]);

        if (entry->name == nullptr) {
            entry->name = name;
            entry->num_deleted = 1;
            return;
        }
        if (entry->name == name) {
            entry->num_deleted++;
            return;
        }
    }

    LOG_ERROR("object_counter_t: MAX_ENTRIES reached.");
#endif
}

void object_counter_summarize(bool const is_final) {
#if ENABLE_OBJECT_COUNTER
    for (size_t i = 0; i < MAX_ENTRIES; i++) {
        entry_t* entry = &(ENTRIES[i]);

        if (entry->name == nullptr) {
            return;
        }

        LOG_INFO("object_counter_t: %s: %zu created, %zu deleted, %zu current.", entry->name, entry->num_created, entry->num_deleted, entry->num_created - entry->num_deleted);
        if (is_final && entry->num_created != entry->num_deleted) {
            LOG_WARN("object_counter_t: %s: %zu not deleted!", entry->name, entry->num_created - entry->num_deleted);
        }
    }
#endif
}