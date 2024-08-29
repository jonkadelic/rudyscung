#pragma once

#ifndef M_PI
#define M_PI   3.14159265358979323846264338327950288
#endif
#define TO_RADIANS(degrees) (degrees * M_PI / 180.0)

#define VEC_ADD_INIT(a, b) { a[AXIS__X] + b[AXIS__X], a[AXIS__Y] + b[AXIS__Y], a[AXIS__Z] + b[AXIS__Z] }
#define VEC_ADD(a, b) ((typeof(a[0])[]) VEC_ADD_INIT(a, b))
#define VEC_SUB_INIT(a, b) { a[AXIS__X] - b[AXIS__X], a[AXIS__Y] - b[AXIS__Y], a[AXIS__Z] - b[AXIS__Z] }
#define VEC_SUB(a, b) ((typeof(a[0])[]) VEC_SUB_INIT(a, b))
#define VEC_CAST_INIT(t, a) { (t) a[AXIS__X], (t) a[AXIS__Y], (t) a[AXIS__Z] }
#define VEC_CAST(t, a) ((t[]) VEC_CAST_INIT(t, a))
#define MIN(a, b) (a > b ? b : a)
#define MAX(a, b) (a > b ? a : b)

char* const strcata(char const* const a, char const* const b);

float const lerp(float const a, float const b, float const t);

float const absf(float const x);

float const map_to_0_1(float const x);

unsigned long const get_time_ms(void);