#pragma once
#include "epd_internals.h"

/* MODE_GL16 (type=5) — same LUT as GC (TB8_24_GC16); my_waveform_gc.h must be included first */

const EpdWaveformPhases* my_wm_gl_ranges[1] = { &my_gc_25_0 };
const EpdWaveformMode my_wm_gl = { .type = 5, .temp_ranges = 1, .range_data = &my_wm_gl_ranges[0] };
