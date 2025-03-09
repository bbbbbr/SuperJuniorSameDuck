#include "gb.h"
#include <time.h>
#include <string.h>
#include <stdio.h>
#include "lodepng.h"

#include "megaduck_laptop_periph.h"

#define NO_SDL_FUNCS  // TODO: FIXME: Hack to allow include in Core code that compiles without SDL in the include path
#include "SDL/megaduck_printer_preview.h"


static void clear_image(void);
static void process_tile(uint8_t * tile_buf);
static void reset_rx_cache(void);
static void handle_gfx_bytes(uint8_t * p_bytes, uint8_t byte_count);
static void handle_page_done(void);
static void do_carriage_return(void);
static void do_line_feed(void);
static bool save_image_to_png(char * filename_out);


// TODO: simulate thermal paper look?
// https://github.com/Raphael-Boichot/GameboyPrinterPaperSimulation

#define PRINTER_DEFAULT_PNG_FILENAME "megaduck_print.png"

// The system ROM sends a query/init printer command on startup,
// don't show the preview window until 2+ commands have come in.
#define INIT_COUNT_THRESHOLD_SHOW_PREVIEW 2

#define PRINTER_TILE_PX_IN_BYTE 8        // 8 horizontal pixels per tile byte
#define PRINTER_TILE_BYTE_SZ    8        // Tiles are 1bpp, 8x8px = 8 bytes
#define PRINTER_TILE_WH         8        // Tile width and height for printing is 8 Pixels
#define PRINTER_WIDTH_TILES     20       // Printer width supported is 20 tiles wide
#define PRINTER_HEIGHT_TILES   (18 + 4)  // Guessing around 4-5 extra carriage returns before/after
#define PRINTER_WIDTH_PX       (PRINTER_TILE_WH * PRINTER_WIDTH_TILES)
#define PRINTER_HEIGHT_PX      (PRINTER_TILE_WH * PRINTER_HEIGHT_TILES)
#define ER_TILE_CACHE_SZ  (PRINTER_TILE_BYTE_SZ * 3)  // Buffer to reconstruct tile byte data from packets (spread over 2 max)

// // Thermal Printer related 
#define PRINTER_CARRIAGE_RETURN      0x0D  // Return print head to start of 8 pixel high row
#define PRINTER_LINE_FEED            0x0A  // Feed printer paper to next 8 pixel high row (there are two print passes per-row [to print different greys], so LF only happens every other row of printing)
#define PRINTER_LEN_5_END_ROW_CR     5     // 5 bytes (4 data bytes + CR + LF)
#define PRINTER_LEN_6_END_ROW_CRLF   6     // 6 bytes (4 data bytes + CR + LF)
#define PRINTER_LEN_12_ROW_DATA      12    // 12 data bytes
#define PRINTER_CR_IDX               (PRINTER_LEN_5_END_ROW_CR - 1)   // Byte number 5
#define PRINTER_LF_IDX               (PRINTER_LEN_6_END_ROW_CRLF - 1) // Byte number 6

// Used in printer init reply
// Assumed it is 1, but won't know until have hardware to test with
// Bit.0
#define PRINTER_INIT_OK   0x01
#define PRINTER_INIT_FAIL 0x00

#define PRINT_TONE_WHITE  0xFF  // Full White
#define PRINT_TONE_MED    0x7F  // 50% grey
#define PRINT_TONE_DARK   0x00  // Full black


#define PRINT_1_PASS_PACKET_TO_BULK_SWITCH_THRESHOLD 4 // switch to non-packetized bulk transfer after 4 packets

enum {
    PRINTER_STATE_RESET,
    PRINTER_STATE_INITIALIZED,
    PRINTER_STATE_PAGE_DONE,
};

typedef struct {
    // MegaDuck Printer IO state and values
    uint8_t  state;
    uint8_t  type;
    int      init_cmd_count;
    int      tilepos_x, tilepos_y;
    int      cache_count;
    int      cache_used;
    int      tile_row_packet_count;
    uint8_t  tile_cache[ER_TILE_CACHE_SZ];
    uint8_t  image[PRINTER_WIDTH_PX * PRINTER_HEIGHT_PX];

} GB_megaduck_printer_t;

static GB_megaduck_printer_t printer = 
    {.state = PRINTER_STATE_RESET,
    .tilepos_x = 0,
    .tilepos_y = 0,
    .cache_count = 0,
    .cache_used = 0,
    .tile_row_packet_count = 0,
    .type  = MEGADUCK_PRINTER_TYPE,
    .init_cmd_count = 0 };


static void clear_image(void) {

    memset(printer.image, PRINT_TONE_WHITE, sizeof(printer.image));
    memset(printer.tile_cache, 0, sizeof(printer.tile_cache));
}


// Return which printer type is in use (Single or Double Pass) without 
// the OK status bit included. For 1 vs 2 Pass protocol handling.
uint8_t MD_printer_get_type(void) {
    return printer.type;
}


// Pass through which adds preview size
void MD_printer_open_preview(void) {
    MD_printer_preview_init(PRINTER_WIDTH_PX, PRINTER_HEIGHT_PX);
}

// Init MegaDuck Laptop Printer
// Init is sent via MEGADUCK_SYS_CMD_PRINT_INIT_MAYBE_EXT_IO
uint8_t MD_printer_init(GB_megaduck_laptop_t * periph) {

    printer.state       = PRINTER_STATE_INITIALIZED;
    printer.type        = MEGADUCK_PRINTER_TYPE;
    printer.tilepos_x   = 0;
    printer.tilepos_y   = 0;
    printer.cache_count = 0;
    printer.cache_used  = 0;
    printer.tile_row_packet_count = 0;
    clear_image();
    
    // The system ROM sends a query/init printer command on startup,
    // don't show the preview window until 2+ commands have come in.
    printer.init_cmd_count++;
    printf("- MD_printer_init #%d\n", printer.init_cmd_count);
    if (printer.init_cmd_count >= INIT_COUNT_THRESHOLD_SHOW_PREVIEW)
        MD_printer_open_preview();

    uint8_t printer_reply = PRINTER_INIT_OK | printer.type;
    return (printer_reply);
}


// Expects
static void process_tile(uint8_t * tile_buf) {

    uint32_t tile_st_px = printer.tilepos_x * PRINTER_TILE_WH;
    uint32_t tile_st_py = printer.tilepos_y * PRINTER_TILE_WH;

    // Tile is flipped horizontally and rotated -90 degrees,
    // so transform to pixels should rotate 90 degrees, then flip horizontal
    //
    // Normal tile:    Received tile: (Note changed axes)
    //    bits          bits
    // b  --X--         --Y--
    // y |  0 .. 7     |   0 .. 7
    // t Y 0           X 0        
    // e | .           | . 
    // s | 7           | 7     

    // TODO: Could optimize this once the idea is proven

    // Render the tile bits into pixels
    // Byte increment
    for (int tile_x= 0; tile_x < PRINTER_TILE_BYTE_SZ; tile_x++) {
        // Bit increment
        uint8_t tile_col = tile_buf[tile_x];
        for (int tile_y = 0; tile_y < PRINTER_TILE_PX_IN_BYTE; tile_y++) {
            // If bit is set the print the pixel            
            if (tile_col & (1 << (7 - tile_y))) {
                uint32_t pixel_index = (tile_st_px + tile_x) + ((tile_st_py + tile_y) * PRINTER_WIDTH_PX);
                // Buffer range check
                if (pixel_index < sizeof(printer.image)) {
                    if (printer.type == MEGADUCK_PRINTER_TYPE_2_PASS)
                        printer.image[pixel_index] -= PRINT_TONE_MED;
                    else
                        printer.image[pixel_index] = PRINT_TONE_DARK;
                } else {
                    // printf("- Printer: WARNING: overflowed image buf x=%d, y=%d\n", printer.tilepos_x, printer.tilepos_y);
                }
            }
        }
    }

    // After tile is processed ("printed")
    // then advance the print head by one tile. 
    if (printer.tilepos_x < PRINTER_WIDTH_TILES) {
        printer.tilepos_x++;
        // printf("- Printer: TILE DONE -> X INC -> OK\n");
    } else {
        // TODO: Not sure what happens with the hardware if it doesn't get the expected Carriage Return at the end of a line
        // In this implementation the print head will just keep overwriting the same tile
        // printf("- Printer: TILE DONE -> X INC -> AT ROW END!\n");
    }

    // printf("MD_printer_update------------\n");
    MD_printer_preview_update(PRINTER_WIDTH_PX, PRINTER_HEIGHT_PX, printer.image);
}


static void reset_rx_cache(void) {
    printer.cache_used = printer.cache_count = 0;
}


static void handle_gfx_bytes(uint8_t * p_bytes, uint8_t byte_count) {
    
    if ((printer.cache_count + byte_count) <= sizeof(printer.tile_cache)) {
        // Transfers will be 1 byte when in Single Pass mode + bulk transfer stage
        if (byte_count == 1)
            printer.tile_cache[printer.cache_count] = *p_bytes;
        else
            memcpy(&(printer.tile_cache[printer.cache_count]), p_bytes, byte_count);
        printer.cache_count += byte_count;

        // Print any available tiles in the buffer
        while ((printer.cache_count - printer.cache_used) >= PRINTER_TILE_BYTE_SZ) {
            process_tile(&printer.tile_cache[printer.cache_used]);
            printer.cache_used += 8;
        }

        // Reset buffer if fully used and drained
        if (printer.cache_used == sizeof(printer.tile_cache)) {
            reset_rx_cache();
        }
    }
}


static void handle_page_done(void) {
    printer.state = PRINTER_STATE_PAGE_DONE;
    save_image_to_png(PRINTER_DEFAULT_PNG_FILENAME);
}


static void do_carriage_return(void) {

    // Reset to start of line
    // printf("- Printer: CR\n");
    printer.tilepos_x = 0;
    // Reset packet count for the tile row so Type 1 printers know when to switch from packets to bulk mode
    printer.tile_row_packet_count = 0;
    // Carriage return seems to expect that all tiles bytes for a row will have been sent (160)
    // So OK to reset the rx cache buffer
    reset_rx_cache();
}


static void do_line_feed(void) {

    if (printer.tilepos_y < PRINTER_HEIGHT_TILES) {
        printer.tilepos_y++;
        // printf("- Printer: LF -> OK (Tile Row %d)\n", printer.tilepos_y);
    } else {
        // printf("- Printer: LF -> AT PAGE END! (Tile Row %d)\n", printer.tilepos_y);
        handle_page_done();
    }

    // There is a lot of inconsistency to when programs init the printer
    // so if printing has started and init has only been sent once, then
    // increment init count and open the printer window this once.
    //
    // The intent here is to avoid popping the printer window on
    // startup when it's not being used.
    //
    // System ROM SPA: 1x startup, 3x before printing
    // System ROM GER: 1x startup, 0x before printing
    // Bilder Lexikon: 1x startup, 1x before printing
    // Data Bank:      1x startup, 0x before printing
    if (printer.init_cmd_count < INIT_COUNT_THRESHOLD_SHOW_PREVIEW) {
        printer.init_cmd_count++;
        MD_printer_open_preview();
    }
}


// Single Pass printer has a special scenario where after the 4 packets of 12 bytes
// the transfer mode switches from multi-buffer to a non-packetized stream of bytes with ACKs                            
bool MD_printer_check_switch_to_bulk_rx(void) {

    return ( (printer.type == MEGADUCK_PRINTER_TYPE_1_PASS) &&
             (printer.tile_row_packet_count >= PRINT_1_PASS_PACKET_TO_BULK_SWITCH_THRESHOLD));
}


void MD_printer_process_bulk_data(GB_megaduck_laptop_t * periph) {

    if (printer.state == PRINTER_STATE_INITIALIZED) {
        if (MD_printer_check_switch_to_bulk_rx() == true) {
            // handle rx of 1 byte
            handle_gfx_bytes(&periph->byte_being_received, 1);
        }
        // else
        //     printf("MD_printer_process_bulk_data: Got rx byte but not in bulk mode!\n");
    }
    // else
    //     printf("MD_printer_process_bulk_data: Got rx byte but printer not initialized!\n");

}


// Mystery: What are the 6 extra bytes for in Single Pass mode?
//          Single Pass: (4 * 12 bytes) + 118 bytes = 166 bytes
//          Double Pass: (13 * 12 bytes) + (4 bytes) = 160 bytes + 1 or 2 bytes for CR and/or LF
//          For tile data each row needs 160 bytes, and the extras
//          aren't set to CR/LF. Seems hardware auto-detects those needed?
void MD_printer_finalize_bulk_data(void) {
    // In addition to completing the tile row
    // these also reset per-tile row tracking vars
    do_carriage_return();
    do_line_feed();
}


// Process data sent to the MegaDuck Laptop Printer (from Duck ROM perspective)
// Bytes are sent via MEGADUCK_SYS_CMD_PRINT_SEND_BYTES
//
void MD_printer_process_buf(GB_megaduck_laptop_t * periph) {
    uint8_t * rxbuf = periph->rx_buffer;

    if (printer.state == PRINTER_STATE_INITIALIZED) {

        switch(periph->rx_buffer_size) {

            case PRINTER_LEN_5_END_ROW_CR:
                handle_gfx_bytes(periph->rx_buffer, PRINTER_LEN_5_END_ROW_CR - 1); // 4 gfx bytes in this packet (1 control byte)
                if (rxbuf[PRINTER_CR_IDX] == PRINTER_CARRIAGE_RETURN) do_carriage_return();
                break;

            case PRINTER_LEN_6_END_ROW_CRLF:
                handle_gfx_bytes(periph->rx_buffer, PRINTER_LEN_6_END_ROW_CRLF - 2); // 4 gfx bytes in this packet (2 control bytes)
                if (rxbuf[PRINTER_CR_IDX] == PRINTER_CARRIAGE_RETURN) do_carriage_return();
                if (rxbuf[PRINTER_LF_IDX] == PRINTER_LINE_FEED)       do_line_feed();
                break;

            case PRINTER_LEN_12_ROW_DATA:
                handle_gfx_bytes(periph->rx_buffer, PRINTER_LEN_12_ROW_DATA);
                // Count packets for when to switch into bulk mode for single pass printer
                printer.tile_row_packet_count++;
                break;

            default:
                printf("MD_print_buf ! UNKNOWN ! Len=%d\n", periph->rx_buffer_size);
                break;
        }
        // printf("MD_print_buf Len=0x%0X (tx=%d, ty=%d)\n", 
        //     periph->rx_buffer_size, printer.tilepos_x, printer.tilepos_y);
    } else {
        // printf("Printer: Got data packets, but printer not ready for them! State = %d\n", printer.state);
    }
}


void MD_printer_save_image_to_png(void) {
    // Note: If save is triggered mid-print job then the image may continue
    //       being transmitted and built since the system ROM doesn't
    //       seem to check for or try to fail if there are errors mid-way
    save_image_to_png(PRINTER_DEFAULT_PNG_FILENAME);
}


// Save an indexed color image in png format
static bool save_image_to_png(char * filename_out) {

    LodePNGState png_state;
    int status = false;
    unsigned int error;
    // unsigned char * p_png = NULL;
    unsigned char * p_png_image = NULL;
    size_t          png_size_bytes = 0;
    // int             c;

    lodepng_state_init(&png_state);

    // Make a 256 color greyscale palette
    for (uint16_t c = 0; c < 256; c++) {
        // printf("PNG adding color %d : %3d, %3d, %3d\n", c, c, c, c);
        lodepng_palette_add(&png_state.info_png.color, c, c, c, 255);  // alpha (fully opaque)
        lodepng_palette_add(&png_state.info_raw, c, c, c, 255);        // alpha (fully opaque)
    }

    // lodepng options: going from RAW to indexed PNG
    png_state.info_raw.colortype = LCT_PALETTE; // TODO: use LCT_GREY instead
    png_state.info_raw.bitdepth = 8;


    // Palette must be added both to input and output color mode, because in this
    // Sample both the raw image and the expected PNG image use that palette.
    png_state.info_png.color.colortype = LCT_PALETTE;
    png_state.info_png.color.bitdepth = 8;
    png_state.encoder.auto_convert = LAC_NO;  // Specify exactly what output PNG color mode we want

   // Encode and save
    error = lodepng_encode(&p_png_image,
                            &png_size_bytes,
                            printer.image,
                            PRINTER_WIDTH_PX,
                            PRINTER_HEIGHT_PX,
                            &png_state);
    if (error) {
        printf("PNG encoder error: %u - %s\n", error, lodepng_error_text(error));
        status = false;
    }
    else {
        printf("Writing output image to png file: %d x %d, %d bytes to %s\n", PRINTER_WIDTH_PX, PRINTER_HEIGHT_PX, (int)png_size_bytes, filename_out);
        lodepng_save_file(p_png_image, png_size_bytes, filename_out);
        status = true;
    }

    // Free resources
    lodepng_state_cleanup(&png_state);

    return status;
}
