#include "./util.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

char* const strcata(char const* const a, char const* const b) {
    char* const result = malloc(strlen(a) + strlen(b) + 1);
    assert(result != nullptr);
    strcpy(result, a);
    strcat(result, b);
    return result;
}