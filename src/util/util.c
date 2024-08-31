#include "./util.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

char* const strcata(char const* const a, char const* const b) {
    char* const result = malloc(strlen(a) + strlen(b) + 1);
    assert(result != nullptr);
    strcpy(result, a);
    strcat(result, b);
    return result;
}

float const lerp(float const a, float const b, float const t) {
    return a + (b - a) * t;
}

float const absf(float const x) {
    return x < 0 ? -x : x;
}

float const map_to_0_1(float const x) {
    if (x >= 1.0f) {
        return x - floor(x);
    } else if (x < 0.0f) {
        return 0;//1 - (x - ceil(x));
    }

    return x;
}

unsigned long const get_time_ms(void) {
    struct timeval time;
    gettimeofday(&time, nullptr);

    return time.tv_sec * 1000 + time.tv_usec / 1000;
}