#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "defs.h"


/* Q: (paraphrased) Is it possible to enable SC internal clock transfer
      with internal clock before loading payload byte into SB?

   A [nitro2k01]: This is actually safe.
    - clock generator for serial hardware unit
      - fed by the 16384 Hz tap of DIV
        - divides it by 2 to create 8192 Hz

      - This divider is reset every time SC is written
      - first pulse that shifts bits
        - happens next negative edge of the output of the divider

    - So you have like 63-127 M cycles from writing SC
      - depending on the current phase of DIV.
    - Ie, first bit of transfer
      - doesn't happen until this divider (COTY, near top left)
        - receives a second 16384 Hz clock pulse
*/
#define DUCK_IMPROVED_SIO_TIMING

// #define MEGADUCK_SYS_SERIAL_LOGGING_ENABLED
// #define MEGADUCK_SYS_SERIAL_LOG_ALL_IN_OUT
// #define MEGADUCK_SYS_SERIAL_LOG_RX_BUFFER
// #define MEGADUCK_SYS_SERIAL_LOG_TX_BUFFER
// #define MEGADUCK_SYS_SERIAL_LOG_TX_BYTES
// #define DEBUG_LOG_DUCK_SERIAL_RX_IDLE_CMD
// #define DEBUG_LOG_DUCK_SERIAL_RX_GETKEYS_CMD

// #define MEGADUCK_SYS_KEYBOARD_LOG_SEND_NON_NULL
// #define MEGADUCK_SYS_FORCE_LOCAL_DATE_TIME_REPLY

// #define GB_LOG_SERIAL_IO_DETAILS

// #define DEBUG_LOG_DUCK_TRANSLATED_IO
// #define DEBUG_LOG_DUCK_SERIAL_IO
// #define DEBUG_LOG_DUCK_SERIAL_IO_BITS
// #define DEBUG_LOG_DUCK_UNHANDLED_0x7FFF_POSSIBLE_BANK_WRITES
// #define DEBUG_LOG_DUCK_SYSROM_MBC_WRITES
// #define DEBUG_LOG_DUCK_SYSROM_SRAM_ACCESS

// #define DEBUG_LOG_DUCK_RTC_COMMANDS
// #define DEBUG_LOG_DUCK_RTC_COMMANDS_DATA

// #define DEBUG_LOG_DUCK_PRINTER_TIMING

#define MEGADUCK_BUF_SZ  256

#define T_STATES_PER_MSEC 4194.304


// Used in printer init reply
// - Optionally supports 2-pass printing (depending on bit .1 of CMD 9 response)
//   - bit.1: 0 = supported, 1 = not supported
enum {
    MEGADUCK_PRINTER_TYPE_2_PASS = 0x00, // Bit.1 = 0  // 13 x 12 byte packets + 1 x 5 or 6 byte packet (with CR and/or LF)
    MEGADUCK_PRINTER_TYPE_1_PASS = 0x02, // Bit.1 = 1  // 3 x 12 byte packets + 118 non-packet bytes
    MEGADUCK_PRINTER_TYPE_DEFAULT = MEGADUCK_PRINTER_TYPE_2_PASS,
    // MEGADUCK_PRINTER_TYPE_DEFAULT = MEGADUCK_PRINTER_TYPE_1_PASS,

    MEGADUCK_PRINTER_BULK_TILE_ROW_RX_SIZE = 118,  // 118 RX bytes for bulk portion of printer tile row (excluding 4 x 12 bytes prologue per tile row)
};

// Unlike the game boy which (reportedly) inits SP to 0x0000, the stack on MegaDuck is
// different.
// - On handhelds it seems to be random-ish at power-up
// - On the laptop the System ROM seems to rely on it being 0xFFFF or equivalent since it
//   does a call and ret (so pushes and pops the stack) before it even initializes it
//   and that does not result in a crash on hardware as a value of 0x0000 would
enum {
    MEGADUCK_LAPTOP_INITIAL_STACK_VALUE = 0xFFFF,
};

typedef struct {
    // Serial IO state and values
    uint8_t  byte_to_send;
    bool     bit_to_send;
    uint8_t  byte_being_received;
    uint8_t  bits_received;

    // External Clock mode states and values
    uint32_t t_states_elapsed;
    int32_t  t_states_till_update;
    uint8_t  ext_clk_send_bit_counter;
    uint16_t ext_clk_send_buf_size;
    uint16_t ext_clk_send_buf_index;
    uint8_t  ext_clk_send_buf[MEGADUCK_BUF_SZ];

    // Peripheral state
    uint8_t state;
    uint8_t key;
    uint8_t key_modifiers;
    uint8_t last_key;
    uint8_t last_key_modifiers;

    int32_t t_states_print_done_timeout;

    time_t time_delta_rtc_vs_host;

    // Multi-byte buffers
    uint8_t rx_buffer_state;
    uint8_t rx_buffer_checksum;
    uint8_t rx_buffer_purpose;
    int     rx_buffer_size;
    int     rx_buffer_count;
    uint8_t rx_buffer[MEGADUCK_BUF_SZ];

    int     rx_bulk_size;
    int     rx_bulk_count;

    // Power-On Init Counter state
    uint8_t init_counter;

} GB_megaduck_laptop_t;

typedef void (*GB_megaduck_laptop_set_time_callback)(GB_gameboy_t *gb, time_t time);
typedef time_t (*GB_megaduck_laptop_get_time_callback)(GB_gameboy_t *gb);



enum {
    // Init counter states
    MEGADUCK_SYS_STATE_INIT_1_WAIT_RX_COUNTER = 1,    // Power-Up default state
    MEGADUCK_SYS_STATE_INIT_2_SEND_RX_COUNT_ACK,      // External Clock
    MEGADUCK_SYS_STATE_INIT_3_WAIT_TX_COUNTER_REQ,
    MEGADUCK_SYS_STATE_INIT_4_SEND_TX_COUNTER,        // External Clock
    MEGADUCK_SYS_STATE_INIT_5_WAIT_TX_COUNT_ACK,
    // Post-init default state
    MEGADUCK_SYS_STATE_INIT_OK_READY,
    // Command reply states
    MEGADUCK_SYS_STATE_REPLY_CMD_PRINT_INIT_EXT_IO,
    // Multi-byte receive states
    MEGADUCK_SYS_STATE_CMD_SET_RTC,                   // External Clock (partial)
    MEGADUCK_SYS_STATE_GET_RTC_TX,                    // External Clock
    MEGADUCK_SYS_STATE_GET_RTC_WAIT_ACK,

    MEGADUCK_SYS_STATE_GET_KEYS_TX,                   // External Clock
    MEGADUCK_SYS_STATE_GET_KEYS_WAIT_ACK,

    MEGADUCK_SYS_STATE_CMD_PRINT_SEND_BYTES,          // External Clock (partial)

    MEGADUCK_SYS_STATE_CMD_PLAYSPEECH,                // External Clock (partial)

    MEGADUCK_SYS_STATE_PRINT_SEND_BULK_RX,                  
    MEGADUCK_SYS_STATE_PRINT_SEND_BULK_ACK,           // External Clock
    MEGADUCK_SYS_STATE_PRINT_SEND_BULK_ROW_END_ACK,   // External Clock

    MEGADUCK_SYS_STATE_REPLY_CMD_RUN_CART_IN_SLOT,    // External Clock

    MEGADUCK_SYS_POWER_ON_RESET  = MEGADUCK_SYS_STATE_INIT_1_WAIT_RX_COUNTER,
    MEGADUCK_SYS_KEEP_INIT_RESET = MEGADUCK_SYS_STATE_INIT_OK_READY,
};

enum {
    MEGADUCK_RX_BUF_1_LEN,
    MEGADUCK_RX_BUF_2_PAYLOAD,
    MEGADUCK_RX_BUF_3_CHECKSUM,
    MEGADUCK_RX_BUF_4_DONE,
    MEGADUCK_RX_BUF_5_FAIL
};

enum {
    MEGADUCK_SYS_INIT_COUNTER_RESET = 0,
    MEGADUCK_SYS_INIT_COUNTER_DONE  = 255,
};

enum {
    MEGADUCK_SYS_REPLY_BOOT_OK       = 0x01,
    MEGADUCK_SYS_REPLY_BOOT_FAIL     = 0x00, // TODO: Not verified on hardware, assumed based on System ROM
    MEGADUCK_SYS_REPLY_BOOT_END_OK   = 0x00, // Anything WITHOUT bit .0 set // TODO: Specific value not verified on hardware 
    MEGADUCK_SYS_REPLY_BOOT_END_FAIL = 0x01, // Anything WITH    bit .0 set // TODO: Specific value not verified on hardware 

    MEGADUCK_SYS_REPLY_CMD_PRINT_INIT_MAYBE_EXT_IO   = 0x01, // 0xFF, // TODO: Not verified on hardware, need to snoop it
    MEGADUCK_SYS_REPLY_SEND_BUFFER_OK          = 0x03, // Verified
    MEGADUCK_SYS_REPLY_SEND_BUFFER_MAYBE_ERROR = 0x06, // Still not sure what this signifies, but failure of some kind

    MEGADUCK_SYS_REPLY_BUFFER_SEND_AND_CHECKSUM_OK   = 0x01,
    MEGADUCK_SYS_REPLY_BUFFER_SEND_AND_CHECKSUM_FAIL = 0x00, // TODO: Not verified on hardware, need to snoop it

    MEGADUCK_SYS_REPLY_NO_CART_IN_SLOT  = 0x06,  // Maybe also some failure during serial IO multi-byte buffer send
    MEGADUCK_SYS_REPLY_MAYBE_KBD_START  = 0x0E,  // Maybe 0x0E ... why 0x04 when logged? Reply at start of a 4 byte keyboard reply packet

    MEGADUCK_SYS_REPLY_PRINTER_BULK_ACK   = 0x01,  // TODO: Not verified on hardware, but also system ROM discards the value. It just wants *any* byte
};


enum {
    MEGADUCK_SYS_CMD_INIT_SEQ_REQUEST         = 0x00,  // Value sent to request the 255..0 countdown sequence (be sent into the serial port)
    MEGADUCK_SYS_CMD_GET_KEYS                 = 0x00,  // Requests a multi-byte buffer with keyboard data from Peripheral
    MEGADUCK_SYS_CMD_DONE_OR_OK               = 0x01,  // TODO: What does this do and why?
    MEGADUCK_SYS_CMD_DONE_OR_OK_AND_SOMETHING = 0x81,  // TODO: Seen this as a keyboard poll done reply instead of 0x01 by the calculator app, not sure what the difference is
    MEGADUCK_SYS_CMD_ABORT_OR_FAIL            = 0x04,  // Used in some failure conditions
    MEGADUCK_SYS_CMD_PLAYSPEECH               = 0x05,  // Play pre-recorded speech samples (range 1-6, no audio enable required). Playback of one sample can be interrupted by request for playback of another sample
    MEGADUCK_SYS_CMD_RUN_CART_IN_SLOT         = 0x08,  // Check if cartridge in slot and run it
    MEGADUCK_SYS_CMD_PRINT_INIT_MAYBE_EXT_IO  = 0x09,  // Query what printers are connected
    MEGADUCK_SYS_CMD_RTC_SET_DATE_AND_TIME    = 0x0B,  // Sets Hardware RTC Date and Time using multi-byte buffer send/TX
    MEGADUCK_SYS_CMD_RTC_GET_DATE_AND_TIME    = 0x0C,  // Requests a multi-byte buffer with RTC data from Peripheral
    MEGADUCK_SYS_CMD_PRINT_SEND_BYTES         = 0x11,  // Send printer data
    MEGADUCK_SYS_CMD_SWITCH_TO_EXT_LINK_PORT  = 0x14,  // Permanently(?) switches link port from Peripheral control ASIC to link port pins on the DB15 connector (11=SCK, 12=SIN, 13=SOUT)
};



// Set RTC command (From Duck -> Peripheral)
//     Values shown are system power-up defaults
//     All values are in BCD format
//         Ex: Month = December = 12th month = 0x12 (NOT 0x0C)
//     Command: (0x0B) SYS_CMD_RTC_SET_DATE_AND_TIME
//     Buffer: 8 Bytes
//         00: Year   : 94 (Year = 1900 + Date Byte in BCD 0x94) Quique Sys Range: 0x92 - 0x11 (1992 - 2011)
//         01: Month  : 01 (January / Enero) TODO: Range: 0x01 - 0x12
//         02: Day    : 01 (1st)
//         03: DoW    : 06 (6th day of week: Saturday / Sabado) TODO: Range: 0x01 -0x07
//         04: AM/PM  : 00 (AM) 0=AM, 1=PM- TODO: Verify
//         05: Hour   : 00 (With above it's: 12 am) TODO: Range 0-11
//         06: Minute : 00
//         07: Second?: 00
enum {
    MEGADUCK_BUF_SZ_RTC = 8,  // 8 bytes YEAR/MON/DAT/DOW/AMPM/HOUR/MIN/SEC (size without len and checksum bytes)

    MEGADUCK_RTC_IDX_YEAR  = 0,
    MEGADUCK_RTC_IDX_MON   = 1,
    MEGADUCK_RTC_IDX_DAY   = 2,
    MEGADUCK_RTC_IDX_DOW   = 3,
    MEGADUCK_RTC_IDX_AMPM  = 4,
    MEGADUCK_RTC_IDX_HOUR  = 5,
    MEGADUCK_RTC_IDX_MIN   = 6,
    MEGADUCK_RTC_IDX_SEC   = 7,
};

// Speech Playback command (From Duck -> Peripheral)
//     Command: (0x05) MEGADUCK_SYS_CMD_PLAYSPEECH
//     Buffer: 1 Bytes
//         00: Speech Phrase Num : Range 1-6
enum {
    MEGADUCK_BUF_SZ_PLAYSPEECH = 1,      // 1 byte (size without len and checksum bytes)
    MEGADUCK_PLAYSPEECH_IDX_SAMPLE_NUM  = 0  // 
};


enum {
    MEGADUCK_SPEECH_CMD_MIN = 1,
    MEGADUCK_SPEECH_CMD_MAX = 6
};

 // In T-States, approximately 8192 Hz to match normal speed serial link with external clock
//
// 1. Clock~Dots per Sec 4194304 / 70224 T-States per Frame = 59.72750057 Frames per Sec
// 2. Clock~Dots per Sec 4194304 / Serial 8192 Hz = ~512 T-States per Serial Clock
// 3. 70224 T-States per Frame   / 512 T-States per Serial Clock = 137.15625 Serial Clocks per Frame
//
#define MEGADUCK_LAPTOP_TICK_DELAY_MSEC(MSECS) (((8192 * 512) / 1000) * MSECS)
enum {
    MEGADUCK_LAPTOP_TICK_COUNT_RESET = 512,
    // The System ROM has a built-in ~1 msec delay after most transmits before
    // it enables externally driven serial transfers and turns on it's SIO interrupt handler.
    // So any serial replies before that minimum time get discarded / are lost
    //
    // Maybe it's to allow for an optional reply/cancel from the System ROM between transmits?
    //
    // Min additional delay seems to be 3 serial clocks -> 0.36 msec : ((3 serial clocks / 8192 hz) × 1000 msec) = 0.36 msec
    // Using 4 (0.48 msec) for now so there is more margin for error
    //
    // TODO: Measure what this tends to be on hardware with a test ROM
    MEGADUCK_LAPTOP_TICK_COUNT_TX_DELAY = (int)MEGADUCK_LAPTOP_TICK_DELAY_MSEC(0.48),

    // Reply to command that initiated the buffer write requires 2+ msec delay for unknown reasons
    // (at least for buffer reads maybe extra delay for RTC latch or keyboard matrix scan on)
    MEGADUCK_LAPTOP_TICK_COUNT_RX_BUF_START = (int)MEGADUCK_LAPTOP_TICK_DELAY_MSEC(2.5),  // TODO: Not verified on hardware, assumed based on System ROM
    MEGADUCK_LAPTOP_TICK_COUNT_TX_BUF_REPLY_START = (int)MEGADUCK_LAPTOP_TICK_DELAY_MSEC(2.5),  // TODO: Not verified on hardware, assumed based on System ROM
    MEGADUCK_LAPTOP_TICK_COUNT_TX_BUF_REPLY_NEXT  = (int)MEGADUCK_LAPTOP_TICK_DELAY_MSEC(2),    // TODO: Not verified on hardware, assumed based on System ROM

    MEGADUCK_LAPTOP_EXT_CLOCK_SEND_INDEX_RESET = 0,

    MEGADUCK_LAPTOP_PRINT_DONE_IDLE = 0,
    MEGADUCK_LAPTOP_PRINT_DONE_STILL_ACTIVE = (int)MEGADUCK_LAPTOP_TICK_DELAY_MSEC(3000),  // 3 seconds seems reasonable

};


// ; RX Bytes for Keyboard Serial Reply
// ; - 1st:
// ;   -  Always 0x04 (Length)
// ; - 2nd:
// ;    - KEY REPEAT : |= 0x01  (so far looks like with no key value set in 3rd Byte)
// ;    - CAPS_LOCK: |= 0x02
// ;    - SHIFT: |= 0x04
// ;    - LEFT_PRINTSCREEN: |= 0x08
// ; - 3rd:
// ;    - Carries the keyboard key scan code
// ;    - 0x00 when no key pressed
// ; - 4th:
// ;     - Two's complement checksum byte
// ;     - It should be: #4 == (((#1 + #2 + #3) XOR 0xFF) + 1) [two's complement]
// ;     - I.E: (#4 + #1 + #2 + #3) == 0x100 -> unsigned overflow -> 0x00
// ;
// ;
// ; - Left /right shift are shared
// ;
// ; Keyboard serial reply scan codes have different ordering than SYS_CHAR_* codes
// ; - They go diagonal down from upper left for the first *4* rows
// ; - The bottom 4 rows (including piano keys) are more varied
// ;
// ; LEFT_PRINTSCREEN 00 + modifier 0x08 ??? Right seems to have actual keycode
enum {
    MEGADUCK_KBD_BUF_REPLY_LEN =  4, // 1 Length Header, 2 Payload, 1 Checksum
    MEGADUCK_RTC_BUF_REPLY_LEN = 10, // 1 Length Header, 8 Payload, 1 Checksum

};

enum {
    MEGADUCK_KBD_CODE_NONE  = 0x00,
    MEGADUCK_KBD_FLAGS_NONE = 0x00,

    // ; Modifier Keys / Flags for RX Byte 2
    // ;
    // ; See input_key_modifier_flags__RAM_D027_
    MEGADUCK_KBD_FLAG_KEY_REPEAT             =  0x01,
    MEGADUCK_KBD_FLAG_KEY_REPEAT_BIT         =  0,
    MEGADUCK_KBD_FLAG_CAPSLOCK               =  0x02,
    MEGADUCK_KBD_FLAG_CAPSLOCK_BIT           =  1,
    MEGADUCK_KBD_FLAG_SHIFT                  =  0x04,
    MEGADUCK_KBD_FLAG_SHIFT_BIT              =  2,
    // ; Right Print Screen seems to have actual scancode vs Left being in a flag
    MEGADUCK_KBD_FLAG_PRINTSCREEN_LEFT       =  0x08,
    MEGADUCK_KBD_FLAG_PRINTSCREEN_LEFT_BIT   =  3,


    // ; RX Byte 3 Flags
    // ; All valid keys seem to have bit 7 set (0x80+)
    MEGADUCK_KBD_KEYCODE_BASE_BIT  =  7,
    MEGADUCK_KBD_KEYCODE_BASE      =  0x80,

    // ; First 4 rows (top of keyboard)  ~ 0x80 - 0xB7
    // ;
    // ; - For each row, most chars are +4 vs char to immediate left
    // ;
    // ; Starting values
    // ; - Row 1: 0x80
    // ; - Row 2: 0x81
    // ; - Row 3: 0x82
    // ; - Row 4: 0x83

    // ; Row 1
    MEGADUCK_KBD_CODE_F1                  =  0x80,
    MEGADUCK_KBD_CODE_F2                  =  0x84,
    MEGADUCK_KBD_CODE_F3                  =  0x88,
    MEGADUCK_KBD_CODE_F4                  =  0x8C,
    MEGADUCK_KBD_CODE_F5                  =  0x90,
    MEGADUCK_KBD_CODE_F6                  =  0x94,
    MEGADUCK_KBD_CODE_F7                  =  0x98,
    MEGADUCK_KBD_CODE_F8                  =  0x9C,
    MEGADUCK_KBD_CODE_F9                  =  0xA0,
    MEGADUCK_KBD_CODE_F10                 =  0xA4,
    MEGADUCK_KBD_CODE_F11                 =  0xA8,
    MEGADUCK_KBD_CODE_F12                 =  0xAC,
    // ; GAP at 0xB0 maybe Blank spot where F13 would be
    // ; GAP at 0xB4 maybe ON Key?

    // ; Row 2
    MEGADUCK_KBD_CODE_ESCAPE              =  0x81,  //; Spanish label: Salida | German label: Esc,
    MEGADUCK_KBD_CODE_1                   =  0x85,  //; Shift alt: !,
    MEGADUCK_KBD_CODE_2                   =  0x89,  //; Shift alt: ",
    MEGADUCK_KBD_CODE_3                   =  0x8D,  //; Shift alt: · (Spanish, mid-dot) | § (German, legal section),
    MEGADUCK_KBD_CODE_4                   =  0x91,  //; Shift alt: $,
    MEGADUCK_KBD_CODE_5                   =  0x95,  //; Shift alt: %,
    MEGADUCK_KBD_CODE_6                   =  0x99,  //; Shift alt: &,
    MEGADUCK_KBD_CODE_7                   =  0x9D,  //; Shift alt: /,
    MEGADUCK_KBD_CODE_8                   =  0xA1,  //; Shift alt: (,
    MEGADUCK_KBD_CODE_9                   =  0xA5,  //; Shift alt: ),
    MEGADUCK_KBD_CODE_0                   =  0xA9,  //; Shift alt: \,
    MEGADUCK_KBD_CODE_SINGLE_QUOTE        =  0xAD,  //; Shift alt: ? (Spanish) | German version: ß (eszett),
    MEGADUCK_KBD_CODE_EXCLAMATION_FLIPPED =  0xB1,  //; Shift alt: ¿ (Spanish) | ` (German),  //; German version: ' (single quote?),
    MEGADUCK_KBD_CODE_BACKSPACE           =  0xB5,  //; German label: Lösch,
    // ; See Continued Row 2 below

    // ; Row 3
    MEGADUCK_KBD_CODE_HELP                =  0x82,  //; Spanish label: Ayuda | German label: Hilfe,
    MEGADUCK_KBD_CODE_Q                   =  0x86,
    MEGADUCK_KBD_CODE_W                   =  0x8A,
    MEGADUCK_KBD_CODE_E                   =  0x8E,
    MEGADUCK_KBD_CODE_R                   =  0x92,
    MEGADUCK_KBD_CODE_T                   =  0x96,
    MEGADUCK_KBD_CODE_Y                   =  0x9A,  //; German version: z,
    MEGADUCK_KBD_CODE_U                   =  0x9E,
    MEGADUCK_KBD_CODE_I                   =  0xA2,
    MEGADUCK_KBD_CODE_O                   =  0xA6,
    MEGADUCK_KBD_CODE_P                   =  0xAA,
    MEGADUCK_KBD_CODE_BACKTICK            =  0xAE,  //; Shift alt: [ (Spanish, only shift mode works) | German version: Ü,
    MEGADUCK_KBD_CODE_RIGHT_SQ_BRACKET    =  0xB2,  //; Shift alt: * | German version: · (mid-dot),
    MEGADUCK_KBD_CODE_ENTER               =  0xB6,  //; Spanish label: Entra | German label: Ein-gabe,
    // ; See Continued Row 3 below

    // ; Row 4
    // ; GAP at 0x83 maybe CAPS LOCK  (Spanish label: Mayuscula, German label: Groß)
    MEGADUCK_KBD_CODE_A                   =  0x87,
    MEGADUCK_KBD_CODE_S                   =  0x8B,
    MEGADUCK_KBD_CODE_D                   =  0x8F,
    MEGADUCK_KBD_CODE_F                   =  0x93,
    MEGADUCK_KBD_CODE_G                   =  0x97,
    MEGADUCK_KBD_CODE_H                   =  0x9B,
    MEGADUCK_KBD_CODE_J                   =  0x9F,
    MEGADUCK_KBD_CODE_K                   =  0xA3,
    MEGADUCK_KBD_CODE_L                   =  0xA7,
    MEGADUCK_KBD_CODE_N_TILDE             =  0xAB,  //; German version: ö,
    MEGADUCK_KBD_CODE_U_UMLAUT            =  0xAF,  //; German version: ä,
    MEGADUCK_KBD_CODE_O_OVER_LINE         =  0xB3,  //; º Masculine ordinal indicator (Spanish) | German version: #  | Shift alt: ª [Feminine ordinal indicator, A over line] (Spanish) | ^ (German),
    // ; ? GAP at 0x87 ?


    // ; Second 4 rows (bottom of keyboard) ~ 0x80 - 0xB7
    // ;
    // ; - For each row, most chars are +4 vs char to immediate left
    // ;
    // ; Starting values
    // ; - Row 5: 0xB8
    // ; - Row 6: 0xB9
    // ; - Row 7: 0xBA
    // ; - Row 8: 0xBB

    // ; Row 5
    MEGADUCK_KBD_CODE_Z                   =  0xB8,  //; German version: y,
    MEGADUCK_KBD_CODE_X                   =  0xBC,
    MEGADUCK_KBD_CODE_C                   =  0xC0,
    MEGADUCK_KBD_CODE_V                   =  0xC4,
    MEGADUCK_KBD_CODE_B                   =  0xC8,
    MEGADUCK_KBD_CODE_N                   =  0xCC,
    MEGADUCK_KBD_CODE_M                   =  0xD0,
    MEGADUCK_KBD_CODE_COMMA               =  0xD4,  //; Shift alt: ;,
    MEGADUCK_KBD_CODE_PERIOD              =  0xD8,  //; Shift alt: :,
    MEGADUCK_KBD_CODE_DASH                =  0xDC,  //; Shift alt: _ | German version: @,
    // ; See Continued Row 5 below
    // ; Row 6 Continued (from below)
    MEGADUCK_KBD_CODE_DELETE              =  0xE0,  //; *  Spanish label: Borrar | German label: Entf.,



    // ; Encoding is less orderly below


    // ; Row 6
    MEGADUCK_KBD_CODE_SPACE               =  0xB9,  //; Spanish label: Espacio | German label (blank),
    // ; Continued Row 5
    MEGADUCK_KBD_CODE_LESS_THAN           =  0xBD,  //; Shift alt: >,
    // ; Continued Row 6
    MEGADUCK_KBD_CODE_PAGE_UP             =  0xC1,  //; Spanish label: Pg Arriba | German label: Zu-rück,
    MEGADUCK_KBD_CODE_PAGE_DOWN           =  0xC5,  //; Spanish label: Pg Abajo | German label: Wei-ter,
    MEGADUCK_KBD_CODE_MEMORY_MINUS        =  0xC9,
    // ; Continued Row 5
    MEGADUCK_KBD_CODE_MEMORY_PLUS         =  0xCD,
    MEGADUCK_KBD_CODE_MEMORY_RECALL       =  0xD1,
    MEGADUCK_KBD_CODE_SQUAREROOT          =  0xD5,

    // ; ** 3x3 Arrow and Math Key area **
    // ; Continued Row 6
    MEGADUCK_KBD_CODE_MULTIPLY            =  0xD9,
    MEGADUCK_KBD_CODE_ARROW_DOWN          =  0xDD,
    MEGADUCK_KBD_CODE_MINUS               =  0xE1,
    // ; Continued Row 3
    MEGADUCK_KBD_CODE_ARROW_LEFT          =  0xE5,
    MEGADUCK_KBD_CODE_EQUALS              =  0xE9,
    MEGADUCK_KBD_CODE_ARROW_RIGHT         =  0xED,
    // ; Continued Row 2
    MEGADUCK_KBD_CODE_DIVIDE              =  0xE4,  //; German version: :,
    MEGADUCK_KBD_CODE_ARROW_UP            =  0xE8,
    MEGADUCK_KBD_CODE_PLUS                =  0xEC,

    // ; Row 7
    // ; Piano Sharp Keys
    MEGADUCK_KBD_CODE_PIANO_DO_SHARP      =  0xBA,
    MEGADUCK_KBD_CODE_PIANO_RE_SHARP      =  0xBE,
    // ; GAP at 0xC2 where there is no key
    MEGADUCK_KBD_CODE_PIANO_FA_SHARP      =  0xC6,
    MEGADUCK_KBD_CODE_PIANO_SOL_SHARP     =  0xCA,
    MEGADUCK_KBD_CODE_PIANO_LA_SHARP      =  0xCE,
    // ; GAP at 0xD2 where there is no key
    // ;
    // ; Octave 2 maybe
    MEGADUCK_KBD_CODE_PIANO_DO_2_SHARP    =  0xD6,
    MEGADUCK_KBD_CODE_PIANO_RE_2_SHARP    =  0xDA,
    // ; Row 6 Continued
    MEGADUCK_KBD_CODE_PRINTSCREEN_RIGHT   =  0xDE,  //; German label: Druck (* Mixed in with piano keys),
    // ; Row 7 Continued
    MEGADUCK_KBD_CODE_PIANO_FA_2_SHARP    =  0xE2,
    MEGADUCK_KBD_CODE_PIANO_SOL_2_SHARP   =  0xE6,
    MEGADUCK_KBD_CODE_PIANO_LA_2_SHARP    =  0xEA,

    // ; Row 8
    // ; Piano Primary Keys
    MEGADUCK_KBD_CODE_PIANO_DO            =  0xBB,
    MEGADUCK_KBD_CODE_PIANO_RE            =  0xBF,
    MEGADUCK_KBD_CODE_PIANO_MI            =  0xC3,
    MEGADUCK_KBD_CODE_PIANO_FA            =  0xC7,
    MEGADUCK_KBD_CODE_PIANO_SOL           =  0xCB,
    MEGADUCK_KBD_CODE_PIANO_LA            =  0xCF,
    MEGADUCK_KBD_CODE_PIANO_SI            =  0xD3,
    MEGADUCK_KBD_CODE_PIANO_DO_2          =  0xD7,
    MEGADUCK_KBD_CODE_PIANO_RE_2          =  0xDB,
    MEGADUCK_KBD_CODE_PIANO_MI_2          =  0xDF,
    MEGADUCK_KBD_CODE_PIANO_FA_2          =  0xE3,
    MEGADUCK_KBD_CODE_PIANO_SOL_2         =  0xE7,
    MEGADUCK_KBD_CODE_PIANO_LA_2          =  0xEB,
    MEGADUCK_KBD_CODE_PIANO_SI_2          =  0xEF,

    MEGADUCK_KBD_CODE_LAST_KEY            =  MEGADUCK_KBD_CODE_PIANO_SI_2,

    // ; Special System Codes? 0xF0+
    MEGADUCK_KBD_CODE_MAYBE_SYST_CODES_START  =  0xF0,
    MEGADUCK_KBD_CODE_MAYBE_RX_NOT_A_KEY      =  0xF6,

};


void GB_connect_megaduck_laptop(GB_gameboy_t *gb,
                        GB_megaduck_laptop_set_time_callback set_time_callback,
                        GB_megaduck_laptop_get_time_callback get_time_callback);
void GB_disconnect_megaduck_laptop(GB_gameboy_t *gb);
void GB_megaduck_laptop_use_alt_initial_stack_value(GB_gameboy_t *gb);
bool GB_megaduck_laptop_is_enabled(GB_gameboy_t *gb);
bool GB_megaduck_laptop_is_not_enabled(GB_gameboy_t *gb);

void GB_megaduck_laptop_key_set(GB_gameboy_t *gb, uint8_t key, uint8_t key_modifiers);
void GB_megaduck_laptop_key_release(GB_gameboy_t *gb, uint8_t key, uint8_t key_modifiers);

void GB_megaduck_laptop_reset(GB_gameboy_t *gb);
void GB_megaduck_laptop_peripheral_update(GB_gameboy_t *gb, uint8_t cycles);

void MD_periph_reset(GB_megaduck_laptop_t * periph, uint8_t target_state);
