#pragma once

#include <stdint.h>

void MD_printer_preview_init(int width, int height);
void MD_printer_preview_update(int width, int height, uint8_t * p_src_indexed_buffer);
void MD_printer_preview_cleanup(void);

// TODO: FIXME: Hack to allow include in Core code that compiles without SDL in the include path
#ifndef NO_SDL_FUNCS
    #include <SDL.h>
    SDL_Window * MD_printer_preview_get_window(void);
#endif
