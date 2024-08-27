#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct random random_t;

random_t* const random_new(uint64_t const seed);

random_t* const random_new_from_time(void);

void random_delete(random_t* const self);

void random_set_seed(random_t* const self, uint64_t const seed);

void random_next_bytes(random_t* const self, size_t const num_bytes, uint8_t bytes[]);

int const random_next_int(random_t* const self);

uint32_t const random_next_int_bounded(random_t* const self, uint32_t const bound);

long const random_next_long(random_t* const self);

bool const random_next_boolean(random_t* const self);

float const random_next_float(random_t* const self);

double const random_next_double(random_t* const self);
