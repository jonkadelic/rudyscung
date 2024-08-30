#pragma once

#include <stddef.h>

#define OBJ_CTR_INC(type) object_counter_increment(#type)
#define OBJ_CTR_DEC(type) object_counter_decrement(#type)

void object_counter_increment(char const* const name);

void object_counter_decrement(char const* const name);

void object_counter_summarize(bool const is_final);