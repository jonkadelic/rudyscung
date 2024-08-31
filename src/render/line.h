#pragma once

#include "src/world/side.h"

void line_init(void);

void line_cleanup(void);

void line_render(float const a[NUM_AXES], float const b[NUM_AXES], float const width, float const color[3]);

void line_render_box(float const min[NUM_AXES], float const max[NUM_AXES], float const width, float const color[3]);
