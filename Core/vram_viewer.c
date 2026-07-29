#include "gb.h"
#include <time.h>
#include <string.h>
#include <stdio.h>
#include "lodepng.h"

#define NO_SDL_FUNCS  // TODO: FIXME: Hack to allow include in Core code that compiles without SDL in the include path
#include "SDL/vram_viewer_window.h"


static void clear_image(void);
static void render_vram(void);


#define VRAM_VIEW_TILE_BYTE_SZ     16       // Tiles are 2bpp, 8x8px = 16 bytes
#define VRAM_VIEW_TILE_WH          8        // Tile width and height for printing is 8 Pixels
#define VRAM_VIEW_WIDTH_TILES      16       // Printer width supported is 20 tiles wide
#define VRAM_VIEW_HEIGHT_TILES    (8 * 3)   // 3  blocks of 16 wide by 8 tall tile blocks
#define VRAM_VIEW_WIDTH_PX        (VRAM_VIEW_TILE_WH * VRAM_VIEW_WIDTH_TILES)
#define VRAM_VIEW_HEIGHT_PX       (VRAM_VIEW_TILE_WH * VRAM_VIEW_HEIGHT_TILES)

#define VRAM_VIEW_INFO_AREA_HEIGHT_PX  48  // TODO


#define DRAW_COL_WHITE    255
#define DRAW_COL_GR_DARK  172
#define DRAW_COL_GR_LIGHT 86
#define DRAW_COL_BLACK    0

const uint8_t GB_PAL_COLS[4] = {
    DRAW_COL_WHITE,     // 0
    DRAW_COL_GR_DARK,   // 1
    DRAW_COL_GR_LIGHT,  // 2
    DRAW_COL_BLACK,     // 3
};


static uint8_t image[VRAM_VIEW_WIDTH_PX * VRAM_VIEW_HEIGHT_PX];

GB_gameboy_t * cached_gb = NULL;


static void render_vram(void) {

    if (cached_gb == NULL) return;

    // Start at base of VRAM (0x8000)
    uint8_t * p_vram = cached_gb->vram;
    
    // Set up BGP Palette lookup
    uint8_t bgp_pal[4];
    bgp_pal[0] = GB_PAL_COLS[  cached_gb->io_registers[GB_IO_BGP]       & 0x03 ];
    bgp_pal[1] = GB_PAL_COLS[ (cached_gb->io_registers[GB_IO_BGP] >> 2) & 0x03 ];
    bgp_pal[2] = GB_PAL_COLS[ (cached_gb->io_registers[GB_IO_BGP] >> 4) & 0x03 ];
    bgp_pal[3] = GB_PAL_COLS[ (cached_gb->io_registers[GB_IO_BGP] >> 6) & 0x03 ];

    uint32_t row_pixel_index = 0;
    for (int row = 0; row < VRAM_VIEW_HEIGHT_TILES; row++) {

        uint32_t row_col_pixel_index = row_pixel_index;
        for (int column = 0; column < VRAM_VIEW_WIDTH_TILES; column++) {

            // Draw a tile
            uint32_t pixel_index = row_col_pixel_index;
            for (int tile_y = 0; tile_y < VRAM_VIEW_TILE_WH; tile_y++) {

                // Draw a tile row
                uint8_t byte_lo = *p_vram;
                uint8_t byte_hi = *(p_vram + 1);
                for (int tile_x = 0; tile_x < VRAM_VIEW_TILE_WH; tile_x++) {
                    // Look up color for the pixel and set it
                    image[pixel_index + tile_x] = bgp_pal[ ((byte_lo & 0x80) >> 7) | ((byte_hi & 0x80) >> 6) ];

                    // Step to next X pixel in byte
                    byte_lo <<= 1;
                    byte_hi <<= 1;
                }
                p_vram += 2;

                // Step to next tile Y row
                pixel_index += VRAM_VIEW_WIDTH_PX;
            }  // end: tile_y loop

            // Step to next tile horizontally
            row_col_pixel_index += VRAM_VIEW_TILE_WH;
        }
        // Step to next row of tiles down
        row_pixel_index += VRAM_VIEW_WIDTH_PX * VRAM_VIEW_TILE_WH;
    }

    vram_viewer_window_update(VRAM_VIEW_WIDTH_PX, VRAM_VIEW_HEIGHT_PX, image);
}


static void clear_image(void) {
    memset(image, DRAW_COL_WHITE, sizeof(image));
}




// Pass through which adds preview size
void vram_viewer_open_window(void) {
    vram_viewer_window_init(VRAM_VIEW_WIDTH_PX, VRAM_VIEW_HEIGHT_PX);
}


void vram_viewer_init(GB_gameboy_t * gb) {
    cached_gb = gb;
    clear_image();    
    vram_viewer_open_window();
    render_vram();
}


// Refresh VRAM Viewer window, if not initialized then initialize
void vram_viewer_refresh(GB_gameboy_t * gb) {

    if (cached_gb == NULL)
        vram_viewer_init(gb);
    else {
        vram_viewer_open_window(); // Re-open the window in case it was closed
        render_vram();
    }
}