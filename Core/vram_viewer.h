#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "defs.h"

void vram_viewer_refresh(GB_gameboy_t * gb);
void vram_viewer_open_window(void);
void vram_viewer_init(GB_gameboy_t * gb);
