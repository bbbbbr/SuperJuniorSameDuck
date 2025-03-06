#include <stdbool.h>
#include <stdint.h>
#include <SDL.h>

#include "Core/megaduck_laptop.h"

bool restore_main_window_context(); // In main.c

static SDL_GLContext  printer_gl_context = NULL;
static SDL_Window   * printer_window = NULL;
static SDL_Renderer * renderer = NULL;
static SDL_Surface  * surface = NULL;


// Make the printer window context active (temporarily) to do the draw update
static bool printer_set_context() {

    if (printer_window) {
        if (SDL_GL_MakeCurrent(printer_window, printer_gl_context) != 0) {
            // printf("SDL_GL_MakeCurrent failed: %s\n", SDL_GetError());
            return false;
        }
    }
    return true;
}


SDL_Window * MD_printer_preview_get_window(void) {
    return printer_window;
}


void MD_printer_preview_init(int width, int height) {

    if (!printer_window) {
        printer_window = SDL_CreateWindow("Duck Printer (click img to save png)", 
                                           SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                                           (width * 2), (height * 2),
                                           SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI); // 0);

        printer_gl_context = SDL_GL_CreateContext(printer_window);
        if (printer_gl_context == NULL) return; // printer_gl_context = nogl? NULL : SDL_GL_CreateContext(printer_window);

        renderer = SDL_CreateRenderer(printer_window, -1, 0);
        surface = SDL_CreateRGBSurface(0, width, height, 8, 0, 0, 0, 0);

        // Greyscale palette
        SDL_Color palette_8bpp[256];
        for (int i = 0; i < 256; i++) 
            palette_8bpp[i].r = palette_8bpp[i].g = palette_8bpp[i].b = i;

        SDL_SetPaletteColors(surface->format->palette, palette_8bpp, 0, 256);

    }
    restore_main_window_context();
}


void MD_printer_preview_update(int width, int height, uint8_t * p_src_indexed_buffer) {

    if (printer_window) {
        // Update surface pixels
        memcpy(surface->pixels, p_src_indexed_buffer, width * height);

        printer_set_context();

        // Render them
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (!texture) {
            return;
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        SDL_DestroyTexture(texture);

        restore_main_window_context();
    }
}


void MD_printer_preview_cleanup(void) {

    if (surface)        SDL_FreeSurface(surface);
    if (renderer)       SDL_DestroyRenderer(renderer);
    if (printer_window) SDL_DestroyWindow(printer_window);

    surface = NULL;
    renderer = NULL;
    printer_window = NULL;  
}
