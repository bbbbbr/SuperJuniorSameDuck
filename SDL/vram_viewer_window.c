#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <SDL.h>

#include "Core/megaduck_laptop.h"
#include "vram_viewer_window.h"

bool restore_main_window_context(); // In main.c

static SDL_GLContext  vram_viewer_gl_context = NULL;
static SDL_Window   * vram_viewer_window = NULL;
static SDL_Renderer * renderer = NULL;
static SDL_Surface  * surface = NULL;


// Make the window context active (temporarily) to do the draw update
static bool vram_viewer_set_context() {

    if (vram_viewer_window) {
        if (SDL_GL_MakeCurrent(vram_viewer_window, vram_viewer_gl_context) != 0) {
            // printf("SDL_GL_MakeCurrent failed: %s\n", SDL_GetError());
            return false;
        }
    }
    return true;
}


SDL_Window * vram_viewer_window_get_window(void) {

    return vram_viewer_window;
}


void vram_viewer_window_init(int width, int height) {

    if (!vram_viewer_window) {
        vram_viewer_window = SDL_CreateWindow("VRAM Viewer (click img to update)", 
                                           SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                                           (width * 2), (height * 2),
                                           SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI); // 0);

        vram_viewer_gl_context = SDL_GL_CreateContext(vram_viewer_window);
        if (vram_viewer_gl_context == NULL) {
            // Fallback to SDL renderer
            renderer = SDL_CreateRenderer(vram_viewer_window, -1, SDL_RENDERER_ACCELERATED);

            // If hardware-accelerated rendering fails, try software rendering
            if (!renderer)
                renderer = SDL_CreateRenderer(vram_viewer_window, -1, SDL_RENDERER_SOFTWARE);

            // If that failed then close out the printer preview window and control vars
            if (!renderer) {
                vram_viewer_window_cleanup();
                return;
            }
        }
        else {
            renderer = SDL_CreateRenderer(vram_viewer_window, -1, 0);
        }

        surface = SDL_CreateRGBSurface(0, width, height, 8, 0, 0, 0, 0);

        // Greyscale palette
        SDL_Color palette_8bpp[256];
        for (int i = 0; i < 256; i++) 
            palette_8bpp[i].r = palette_8bpp[i].g = palette_8bpp[i].b = i;

        SDL_SetPaletteColors(surface->format->palette, palette_8bpp, 0, 256);
    }
    restore_main_window_context();
}


void vram_viewer_window_update(int width, int height, uint8_t * p_src_indexed_buffer) {

    if (vram_viewer_window) {
        // Update surface pixels
        memcpy(surface->pixels, p_src_indexed_buffer, width * height);

        vram_viewer_set_context();

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


void vram_viewer_window_cleanup(void) {

    if (surface)                SDL_FreeSurface(surface);
    if (renderer)               SDL_DestroyRenderer(renderer);
    if (vram_viewer_gl_context) SDL_GL_DeleteContext(vram_viewer_gl_context);
    if (vram_viewer_window)     SDL_DestroyWindow(vram_viewer_window);

    surface = NULL;
    renderer = NULL;
    vram_viewer_gl_context = NULL;
    vram_viewer_window = NULL;  
}

// Just a shim right now
void vram_viewer_window_refresh(void) {
}