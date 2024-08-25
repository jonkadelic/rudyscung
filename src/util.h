#pragma once

#ifndef M_PI
#define M_PI   3.14159265358979323846264338327950288
#endif
#define TO_RADIANS(degrees) (degrees * M_PI / 180.0)

char* const strcata(char const* const a, char const* const b);

float const lerp(float const a, float const b, float const t);