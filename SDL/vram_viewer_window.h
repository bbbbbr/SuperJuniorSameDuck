#pragma once

#include <stdint.h>

void vram_viewer_window_init(int width, int height);
void vram_viewer_window_update(int width, int height, uint8_t * p_src_indexed_buffer);
void vram_viewer_window_cleanup(void);
void vram_viewer_window_refresh(void);

// TODO: FIXME: Hack to allow include in Core code that compiles without SDL in the include path
#ifndef NO_SDL_FUNCS
    #include <SDL.h>
    SDL_Window * vram_viewer_window_get_window(void);
#endif
