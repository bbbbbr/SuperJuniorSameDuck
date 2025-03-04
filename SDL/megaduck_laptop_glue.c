#include <stdbool.h>
#include <stdint.h>
#include <SDL.h>

#include "Core/megaduck_laptop.h"


#define ENABLE_SHIFT *key_modifiers |=  MEGADUCK_KBD_FLAG_SHIFT
#define BLOCK_SHIFT  *key_modifiers &= ~MEGADUCK_KBD_FLAG_SHIFT

// https://wiki.libsdl.org/SDL2/SDL_Scancode
// https://wiki.libsdl.org/SDL2/SDL_EventType


#define IS_UTF8_1_BYTE(c) (((unsigned char)c & 0b10000000u) == 0u)
#define IS_UTF8_2_BYTE(c) (((unsigned char)c & 0b11100000u) == 0b11000000u)
#define TO_UTF8(c1,c2)    ((((uint16_t)c1 & 0x1Fu) << 6) | ((uint16_t)c2 & 0x3Fu))


// TODO: DUCK: just split this into separate _textinput vs scancode functions
void megaduck_laptop_SDLscancode_to_key(SDL_Event event, uint8_t * key, uint8_t * key_modifiers) {

    if (event.type == SDL_TEXTINPUT) {

        // size_t len = strlen(event.text.text);
        // printf("SDL_TEXTINPUT, len=%d, str=%s (0x%02x)\n", (int)len, event.text.text, (unsigned char)event.text.text[0]);

        // There might be multiple characters, but for now just extract and use the first

        if (IS_UTF8_2_BYTE(event.text.text[0]))  {
            // https://wiki.libsdl.org/SDL2/SDL_TextInputEvent
            // The string is in UTF-8 format, and if split, SDL guarantees that it will not split
            // in the middle of a UTF-8 sequence, so any event will only contain complete codepoints.
            uint16_t codepoint = TO_UTF8(event.text.text[0], event.text.text[1]);
            // printf(" >> 0x%02x , 0x%02x -> 0x%04x\n", (unsigned char)event.text.text[0] & 0x1Fu, (unsigned char)event.text.text[1] & 0x3Fu, codepoint);

            switch (codepoint) {
                // Allow ROM to handle case adjustment for alphabet letters when relevant (so no enable/force of shift)
                // ·
                case 0x00B7: *key = MEGADUCK_KBD_CODE_3;                   ENABLE_SHIFT; return;  // Shift alt: · (Spanish, mid-dot) | § (German, legal section),
                // ¡
                case 0x00A1: *key = MEGADUCK_KBD_CODE_EXCLAMATION_FLIPPED; BLOCK_SHIFT;  return;  // Shift alt: ¿ (Spanish) | ` (German),  // German version: ' (single quote?),
                // ¿
                case 0x00BF: *key = MEGADUCK_KBD_CODE_EXCLAMATION_FLIPPED; ENABLE_SHIFT; return;  // Shift alt: ¿ (Spanish) | ` (German),  // German version: ' (single quote?),
                // º
                case 0x00BA: *key = MEGADUCK_KBD_CODE_O_OVER_LINE;                       return;  // º Masculine ordinal indicator (Spanish) | German version: #  | Shift alt: ª [Feminine ordinal indicator, A over line] (Spanish) | ^ (German),
                // Ü, ü
                case 0x00DC: case 0x00FC: *key = MEGADUCK_KBD_CODE_U_UMLAUT;             return;  // German version: ä,
                // Ñ, ñ
                case 0x00D1: case 0x00F1: *key = MEGADUCK_KBD_CODE_N_TILDE;              return;  // German version: ö,

                default: break;
            }

        } if (IS_UTF8_1_BYTE(event.text.text[0]))  {

            switch (event.text.text[0]) {

                // Case statements are organized by physical keyboard layout / hardware scancode ordering

                // Row 1
                // All Function Keys handled as SDL Scancode instead

                // Row 2
                // Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_ESCAPE // Spanish label: Salida | German label: Esc
                case '1':  *key = MEGADUCK_KBD_CODE_1;            BLOCK_SHIFT; return;  // Shift alt: !,
                case '2':  *key = MEGADUCK_KBD_CODE_2;            BLOCK_SHIFT; return;  // Shift alt: ",
                case '3':  *key = MEGADUCK_KBD_CODE_3;            BLOCK_SHIFT; return;  // Shift alt: · (Spanish, mid-dot) | § (German, legal section),
                case '4':  *key = MEGADUCK_KBD_CODE_4;            BLOCK_SHIFT; return;  // Shift alt: $,
                case '5':  *key = MEGADUCK_KBD_CODE_5;            BLOCK_SHIFT; return;  // Shift alt: %,
                case '6':  *key = MEGADUCK_KBD_CODE_6;            BLOCK_SHIFT; return;  // Shift alt: &,
                case '7':  *key = MEGADUCK_KBD_CODE_7;            BLOCK_SHIFT; return;  // Shift alt: /,
                case '8':  *key = MEGADUCK_KBD_CODE_8;            BLOCK_SHIFT; return;  // Shift alt: (,
                case '9':  *key = MEGADUCK_KBD_CODE_9;            BLOCK_SHIFT; return;  // Shift alt: ),
                case '0':  *key = MEGADUCK_KBD_CODE_0;            BLOCK_SHIFT; return;  // Shift alt: \,
                case '\'': *key = MEGADUCK_KBD_CODE_SINGLE_QUOTE; BLOCK_SHIFT; return;  // Shift alt: ? (Spanish) | German version: ß (eszett),
                // See 2-byte UTF-8 handling above: case '¡':  *key = MEGADUCK_KBD_CODE_EXCLAMATION_FLIPPED; BLOCK_SHIFT; return;  // Shift alt: ¿ (Spanish) | ` (German),  // German version: ' (single quote?),
                // Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_BACKSPACE // German label: Lösch,
                    // Shift Alternates
                    case '!':  *key = MEGADUCK_KBD_CODE_1;            ENABLE_SHIFT; return;  // Shift alt: !,
                    case '"':  *key = MEGADUCK_KBD_CODE_2;            ENABLE_SHIFT; return;  // Shift alt: ",
                    // See 2-byte UTF-8 handling above:  case '·':  *key = MEGADUCK_KBD_CODE_3;            ENABLE_SHIFT; return;  // Shift alt: · (Spanish, mid-dot) | § (German, legal section),
                    case '$':  *key = MEGADUCK_KBD_CODE_4;            ENABLE_SHIFT; return;  // Shift alt: $,
                    case '%':  *key = MEGADUCK_KBD_CODE_5;            ENABLE_SHIFT; return;  // Shift alt: %,
                    case '&':  *key = MEGADUCK_KBD_CODE_6;            ENABLE_SHIFT; return;  // Shift alt: &,
                    case '/':  *key = MEGADUCK_KBD_CODE_7;            ENABLE_SHIFT; return;  // Shift alt: /,
                    case '(':  *key = MEGADUCK_KBD_CODE_8;            ENABLE_SHIFT; return;  // Shift alt: (,
                    case ')':  *key = MEGADUCK_KBD_CODE_9;            ENABLE_SHIFT; return;  // Shift alt: ),
                    case '\\': *key = MEGADUCK_KBD_CODE_0;            ENABLE_SHIFT; return;  // Shift alt: \,
                    case '?':  *key = MEGADUCK_KBD_CODE_SINGLE_QUOTE; ENABLE_SHIFT; return;  // Shift alt: ? (Spanish) | German version: ß (eszett),
                    // See 2-byte UTF-8 handling above: case '¿':  *key = MEGADUCK_KBD_CODE_EXCLAMATION_FLIPPED; ENABLE_SHIFT; return;  // Shift alt: ¿ (Spanish) | ` (German),  // German version: ' (single quote?),

                // Row 3
                // Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_HELP // Spanish label: Ayuda | German label: Hilfe,
                // Allow ROM to handle case adjustment for alphabet letters (so no enable/force of shift)
                case 'Q': case 'q': *key = MEGADUCK_KBD_CODE_Q; return;
                case 'W': case 'w': *key = MEGADUCK_KBD_CODE_W; return;
                case 'E': case 'e': *key = MEGADUCK_KBD_CODE_E; return;
                case 'R': case 'r': *key = MEGADUCK_KBD_CODE_R; return;
                case 'T': case 't': *key = MEGADUCK_KBD_CODE_T; return;
                case 'Y': case 'y': *key = MEGADUCK_KBD_CODE_Y; return;  //  ; German version: z,
                case 'U': case 'u': *key = MEGADUCK_KBD_CODE_U; return;
                case 'I': case 'i': *key = MEGADUCK_KBD_CODE_I; return;
                case 'O': case 'o': *key = MEGADUCK_KBD_CODE_O; return;
                case 'P': case 'p': *key = MEGADUCK_KBD_CODE_P; return;
                case '`':  *key = MEGADUCK_KBD_CODE_BACKTICK;         BLOCK_SHIFT; return;  // Shift alt: [ (Spanish, only shift mode works) | German version: Ü,
                case ']':  *key = MEGADUCK_KBD_CODE_RIGHT_SQ_BRACKET; BLOCK_SHIFT; return;  // Shift alt: * | German version: · (mid-dot),
                // Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_ENTER // Spanish label: Entra | German label: Ein-gabe,
                    // Shift Alternates
                    case '[':  *key = MEGADUCK_KBD_CODE_BACKTICK;         ENABLE_SHIFT; return;  // Shift alt: [ (Spanish, only shift mode works) | German version: Ü,
                    case '*':  *key = MEGADUCK_KBD_CODE_RIGHT_SQ_BRACKET; ENABLE_SHIFT; return;  // Shift alt: * | German version: · (mid-dot),
                // See Continued Row 3 below

                // Row 4
                // GAP at 0x83 maybe CAPS LOCK  (Spanish label: Mayuscula, German label: Groß)
                // Allow ROM to handle case adjustment for alphabet letters (so no enable/force of shift)
                case 'A': case 'a': *key = MEGADUCK_KBD_CODE_A; return;
                case 'S': case 's': *key = MEGADUCK_KBD_CODE_S; return;
                case 'D': case 'd': *key = MEGADUCK_KBD_CODE_D; return;
                case 'F': case 'f': *key = MEGADUCK_KBD_CODE_F; return;
                case 'G': case 'g': *key = MEGADUCK_KBD_CODE_G; return;
                case 'H': case 'h': *key = MEGADUCK_KBD_CODE_H; return;
                case 'J': case 'j': *key = MEGADUCK_KBD_CODE_J; return;
                case 'K': case 'k': *key = MEGADUCK_KBD_CODE_K; return;
                case 'L': case 'l': *key = MEGADUCK_KBD_CODE_L; return;
                // See 2-byte UTF-8 handling above: case 'Ñ': case 'ñ': *key = MEGADUCK_KBD_CODE_N_TILDE;     return;  // German version: ö,
                // See 2-byte UTF-8 handling above: case 'Ö': case 'ö': *key = MEGADUCK_KBD_CODE_U_UMLAUT;    return;  // German version: ä,
                // See 2-byte UTF-8 handling above: case 'º':           *key = MEGADUCK_KBD_CODE_O_OVER_LINE; return;  // º Masculine ordinal indicator (Spanish) | German version: #  | Shift alt: ª [Feminine ordinal indicator, A over line] (Spanish) | ^ (German),
                // ? GAP at 0x87 ?

                // Row 5
                // Allow ROM to handle case adjustment for alphabet letters (so no enable/force of shift)
                case 'Z': case 'z': *key = MEGADUCK_KBD_CODE_Z; return;  // German version: y,
                case 'X': case 'x': *key = MEGADUCK_KBD_CODE_X; return;
                case 'C': case 'c': *key = MEGADUCK_KBD_CODE_C; return;
                case 'V': case 'v': *key = MEGADUCK_KBD_CODE_V; return;
                case 'B': case 'b': *key = MEGADUCK_KBD_CODE_B; return;
                case 'N': case 'n': *key = MEGADUCK_KBD_CODE_N; return;
                case 'M': case 'm': *key = MEGADUCK_KBD_CODE_M; return;
                case ',':  *key = MEGADUCK_KBD_CODE_COMMA;      BLOCK_SHIFT; return;  // Shift alt: ;,
                case '.':  *key = MEGADUCK_KBD_CODE_PERIOD;     BLOCK_SHIFT; return;  // Shift alt: :,
                case '-':  *key = MEGADUCK_KBD_CODE_DASH;       BLOCK_SHIFT; return;  // Shift alt: _ | German version: @,
                    // Shift Alternates
                    case ';':  *key = MEGADUCK_KBD_CODE_COMMA;      ENABLE_SHIFT; return;  // Shift alt: ;,
                    case ':':  *key = MEGADUCK_KBD_CODE_PERIOD;     ENABLE_SHIFT; return;  // Shift alt: :,
                    case '_':  *key = MEGADUCK_KBD_CODE_DASH;       ENABLE_SHIFT; return;  // Shift alt: _ | German version: @,
                // Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_DELETE    // *  Spanish label: Borrar | German label: Entf.,
                // See Continued Row 5 below
                // Row 6 Continued (from below)

                // Row 6
                case ' ':  *key = MEGADUCK_KBD_CODE_SPACE; return;  // Spanish label: Espacio | German label (blank),

                // Continued Row 5
                case '<':  *key = MEGADUCK_KBD_CODE_LESS_THAN; BLOCK_SHIFT; return; // Shift alt: >,
                    // Shift Alternates
                    case '>':  *key = MEGADUCK_KBD_CODE_LESS_THAN; ENABLE_SHIFT; return; // Shift alt: >,

                // Continued Row 6
                // Key handled as SDL Scancode instead: SDLK_PAGEUP:       *key = MEGADUCK_KBD_CODE_PAGE_UP;     return; // Spanish label: Pg Arriba | German label: Zu-rück,
                // Key handled as SDL Scancode instead: SDLK_PAGEDOWN:     *key = MEGADUCK_KBD_CODE_PAGE_DOWN;   return; // Spanish label: Pg Abajo | German label: Wei-ter,

                // TODO: Not yet assigned input: MEGADUCK_KBD_CODE_MEMORY_MINUS        //=  0xC9,
                // Continued Row 5
                // TODO: Not yet assigned input: MEGADUCK_KBD_CODE_MEMORY_PLUS         //=  0xCD,
                // TODO: Not yet assigned input: MEGADUCK_KBD_CODE_MEMORY_RECALL       //=  0xD1,
                // TODO: Not yet assigned input: MEGADUCK_KBD_CODE_SQUAREROOT          //=  0xD5,

                // ** 3x3 Arrow and Math Key area **
                // Continued Row 6
                // Key handled as SDL Scancode instead: case SDLK_KP_MULTIPLY:  *key = MEGADUCK_KBD_CODE_MULTIPLY;    BLOCK_SHIFT; return;
                // Key handled as SDL Scancode instead: case SDLK_DOWN:         *key = MEGADUCK_KBD_CODE_ARROW_DOWN;  BLOCK_SHIFT; return;
                // Key handled as SDL Scancode instead: case SDLK_KP_MINUS:     *key = MEGADUCK_KBD_CODE_MINUS;       BLOCK_SHIFT; return;
                // Continued Row 3
                // Key handled as SDL Scancode instead: case SDLK_LEFT:         *key = MEGADUCK_KBD_CODE_ARROW_LEFT;  BLOCK_SHIFT; return;
                // Key handled as SDL Scancode instead: case SDLK_KP_EQUALS:    *key = MEGADUCK_KBD_CODE_EQUALS;      BLOCK_SHIFT; return;
                case '=': *key = MEGADUCK_KBD_CODE_EQUALS;      BLOCK_SHIFT; return;
                // Key handled as SDL Scancode instead: case SDLK_RIGHT:        *key = MEGADUCK_KBD_CODE_ARROW_RIGHT; BLOCK_SHIFT; return;
                // Continued Row 2
                // Key handled as SDL Scancode instead: case SDLK_KP_DIVIDE:    *key = MEGADUCK_KBD_CODE_DIVIDE;      BLOCK_SHIFT; return;
                // Key handled as SDL Scancode instead: case SDLK_UP:           *key = MEGADUCK_KBD_CODE_ARROW_UP;    BLOCK_SHIFT; return;
                case '+': *key = MEGADUCK_KBD_CODE_PLUS;        BLOCK_SHIFT; return;

                // Row 7
                // Piano Sharp Keys
                // Piano Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_PIANO_DO_SHARP      //=  0xBA,
                // Piano Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_PIANO_RE_SHARP      //=  0xBE,
                // GAP at 0xC2 where there is no key
                // Piano Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_PIANO_FA_SHARP      //=  0xC6,
                // Piano Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_PIANO_SOL_SHARP     //=  0xCA,
                // Piano Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_PIANO_LA_SHARP      //=  0xCE,
                // GAP at 0xD2 where there is no key
                //     // Octave 2 maybe
                // Piano Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_PIANO_DO_2_SHARP    //=  0xD6,
                // Piano Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_PIANO_RE_2_SHARP    //=  0xDA,
                // Row 6 Continued
                // Input handled by the Flag Version instead of key: MEGADUCK_KBD_CODE_PRINTSCREEN_RIGHT   //=  0xDE,  // German label: Druck (* Mixed in with piano keys),
                // Row 7 Continued
                // Piano Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_PIANO_FA_2_SHARP    //=  0xE2,
                // Piano Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_PIANO_SOL_2_SHARP   //=  0xE6,
                // Piano Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_PIANO_LA_2_SHARP    //=  0xEA,

                // Row 8
                // Piano Primary Keys
                // Piano Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_PIANO_DO            //=  0xBB,
                // Piano Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_PIANO_RE            //=  0xBF,
                // Piano Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_PIANO_MI            //=  0xC3,
                // Piano Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_PIANO_FA            //=  0xC7,
                // Piano Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_PIANO_SOL           //=  0xCB,
                // Piano Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_PIANO_LA            //=  0xCF,
                // Piano Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_PIANO_SI            //=  0xD3,
                // Piano Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_PIANO_DO_2          //=  0xD7,
                // Piano Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_PIANO_RE_2          //=  0xDB,
                // Piano Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_PIANO_MI_2          //=  0xDF,
                // Piano Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_PIANO_FA_2          //=  0xE3,
                // Piano Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_PIANO_SOL_2         //=  0xE7,
                // Piano Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_PIANO_LA_2          //=  0xEB,
                // Piano Key handled as SDL Scancode instead: MEGADUCK_KBD_CODE_PIANO_SI_2          //=  0xEF,

                default: break;
            } // switch (event.text.text[0])
        } // if (IS_UTF8_1_BYTE(event.text.text[0]))

    } // if (event.type == SDL_TEXTINPUT)
    else if ((event.type == SDL_KEYDOWN) || (event.type == SDL_KEYUP)) {

        // printf("%s: sym = %d, scancode=%d\n", (event.type == SDL_KEYDOWN) ? "SDL_KEYDOWN" : "SDL_KEYUP", event.key.keysym.sym, event.key.keysym.scancode);
        // Default to no key, no modifiers
        // *key = MEGADUCK_KBD_CODE_NONE;

        // Default to no modifiers
        *key_modifiers = MEGADUCK_KBD_FLAGS_NONE;

        if (event.key.keysym.mod & KMOD_CAPS)
            *key_modifiers |= MEGADUCK_KBD_FLAG_CAPSLOCK; // Capslock is down
        if (event.key.keysym.mod & KMOD_SHIFT)
            *key_modifiers |= MEGADUCK_KBD_FLAG_SHIFT;    // A Shift key is down

        // Why two printscreens on the keyboard?! Right key is a scancode vs. Left key is a flag.
        // Only going to emulate one, the flag version.
        // Added scrolllock since sometimes OS captures and doesn't pass on Printscreen
        //
        // Turned off for now and instead the non-flag printscreen key is used
        // if ((event.key.keysym.scancode == SDL_SCANCODE_PRINTSCREEN) ||
        //     (event.key.keysym.scancode == SDL_SCANCODE_SCROLLLOCK))
        //     *key_modifiers = MEGADUCK_KBD_FLAG_PRINTSCREEN_LEFT; // Printscreen is down

        // Piano keys handled as CTRL-Function keys
        // When CTRL is held, it blocks character conversion for SDL_TEXTINPUT events,
        // so it's possible to capture the scancodes for those characters here
        if (event.key.keysym.mod & KMOD_CTRL) {

            // printf(" CTRL-PIANO: %s: sym = %d, scancode=%d\n", (event.type == SDL_KEYDOWN) ? "SDL_KEYDOWN" : "SDL_KEYUP", event.key.keysym.sym, event.key.keysym.scancode);

            switch (event.key.keysym.scancode) {
                // Row 7
                // Piano Sharp Keys (F1-F10)
                case SDL_SCANCODE_F1:  *key = MEGADUCK_KBD_CODE_PIANO_DO_SHARP; return;
                case SDL_SCANCODE_F2:  *key = MEGADUCK_KBD_CODE_PIANO_RE_SHARP; return;
                // GAP at 0xC2 where there is no key
                case SDL_SCANCODE_F3:  *key = MEGADUCK_KBD_CODE_PIANO_FA_SHARP; return;
                case SDL_SCANCODE_F4:  *key = MEGADUCK_KBD_CODE_PIANO_SOL_SHARP; return;
                case SDL_SCANCODE_F5:  *key = MEGADUCK_KBD_CODE_PIANO_LA_SHARP; return;
                // GAP at 0xD2 where there is no key
                //     // Octave 2 maybe
                case SDL_SCANCODE_F6:  *key = MEGADUCK_KBD_CODE_PIANO_DO_2_SHARP; return;
                case SDL_SCANCODE_F7:  *key = MEGADUCK_KBD_CODE_PIANO_RE_2_SHARP; return;
                // Row 7 Continued
                case SDL_SCANCODE_F8:  *key = MEGADUCK_KBD_CODE_PIANO_FA_2_SHARP; return;
                case SDL_SCANCODE_F9:  *key = MEGADUCK_KBD_CODE_PIANO_SOL_2_SHARP; return;
                case SDL_SCANCODE_F10: *key = MEGADUCK_KBD_CODE_PIANO_LA_2_SHARP; return;

                // Row 8
                // Piano Primary Keys (upper row of keyboard `1234567890-=<backspace>)
                case SDL_SCANCODE_GRAVE:     *key = MEGADUCK_KBD_CODE_PIANO_DO; return;
                case SDL_SCANCODE_1:         *key = MEGADUCK_KBD_CODE_PIANO_RE; return;
                case SDL_SCANCODE_2:         *key = MEGADUCK_KBD_CODE_PIANO_MI; return;
                case SDL_SCANCODE_3:         *key = MEGADUCK_KBD_CODE_PIANO_FA; return;
                case SDL_SCANCODE_4:         *key = MEGADUCK_KBD_CODE_PIANO_SOL; return;
                case SDL_SCANCODE_5:         *key = MEGADUCK_KBD_CODE_PIANO_LA; return;
                case SDL_SCANCODE_6:         *key = MEGADUCK_KBD_CODE_PIANO_SI; return;
                case SDL_SCANCODE_7:         *key = MEGADUCK_KBD_CODE_PIANO_DO_2; return;
                case SDL_SCANCODE_8:         *key = MEGADUCK_KBD_CODE_PIANO_RE_2; return;
                case SDL_SCANCODE_9:         *key = MEGADUCK_KBD_CODE_PIANO_MI_2; return;
                case SDL_SCANCODE_0:         *key = MEGADUCK_KBD_CODE_PIANO_FA_2; return;
                case SDL_SCANCODE_MINUS:     *key = MEGADUCK_KBD_CODE_PIANO_SOL_2; return;
                case SDL_SCANCODE_EQUALS:    *key = MEGADUCK_KBD_CODE_PIANO_LA_2; return;
                case SDL_SCANCODE_BACKSPACE: *key = MEGADUCK_KBD_CODE_PIANO_SI_2; return;

                default: break;
                }
        }


        // Scancode translation for keys that don't come through as translated unicode characters
        switch (event.key.keysym.scancode) {
            case SDL_SCANCODE_TAB:    *key = MEGADUCK_KBD_CODE_HELP; return;
            case SDL_SCANCODE_KP_TAB: *key = MEGADUCK_KBD_CODE_HELP; return;

            // Row 1
            case SDL_SCANCODE_F1:  *key = MEGADUCK_KBD_CODE_F1;  return;
            case SDL_SCANCODE_F2:  *key = MEGADUCK_KBD_CODE_F2;  return;
            case SDL_SCANCODE_F3:  *key = MEGADUCK_KBD_CODE_F3;  return;
            case SDL_SCANCODE_F4:  *key = MEGADUCK_KBD_CODE_F4;  return;
            case SDL_SCANCODE_F5:  *key = MEGADUCK_KBD_CODE_F5;  return;
            case SDL_SCANCODE_F6:  *key = MEGADUCK_KBD_CODE_F6;  return;
            case SDL_SCANCODE_F7:  *key = MEGADUCK_KBD_CODE_F7;  return;
            case SDL_SCANCODE_F8:  *key = MEGADUCK_KBD_CODE_F8;  return;
            case SDL_SCANCODE_F9:  *key = MEGADUCK_KBD_CODE_F9;  return;
            case SDL_SCANCODE_F10: *key = MEGADUCK_KBD_CODE_F10; return;
            case SDL_SCANCODE_F11: *key = MEGADUCK_KBD_CODE_F11; return;
            // case SDL_SCANCODE_F12: *key = MEGADUCK_KBD_CODE_F12; return; TODO, find alternative key maybe (1073741942) - app menu key?

            // Row 2
            case SDL_SCANCODE_ESCAPE:        *key = MEGADUCK_KBD_CODE_ESCAPE;     return; // Spanish label: Salida | German label: Esc
            case SDL_SCANCODE_BACKSPACE:     *key = MEGADUCK_KBD_CODE_BACKSPACE;  return; // German label: Lösch,

            // Row 3
            case SDL_SCANCODE_RETURN:        *key = MEGADUCK_KBD_CODE_ENTER;      return;// Spanish label: Entra | German label: Ein-gabe,
            case SDL_SCANCODE_KP_ENTER:      *key = MEGADUCK_KBD_CODE_ENTER;      return;// Spanish label: Entra | German label: Ein-gabe,

            // Row 5
            case SDL_SCANCODE_DELETE:        *key = MEGADUCK_KBD_CODE_DELETE;     return; // *  Spanish label: Borrar | German label: Entf.,

            // Row 6
            case SDL_SCANCODE_PAGEUP:       *key = MEGADUCK_KBD_CODE_PAGE_UP;     return; // Spanish label: Pg Arriba | German label: Zu-rück,
            case SDL_SCANCODE_PAGEDOWN:     *key = MEGADUCK_KBD_CODE_PAGE_DOWN;   return; // Spanish label: Pg Abajo | German label: Wei-ter,

            // Continued Row 6
            // TODO: To allow KP specific keys to work as KP keys to be sent as MegaDuck specific keys
            // such as KP "divide" key, then may need to capture those first and block the subsequent
            // converted version of them (looks like SDL does let them through)
            case SDL_SCANCODE_KP_MULTIPLY:  *key = MEGADUCK_KBD_CODE_MULTIPLY;    return;
            case SDL_SCANCODE_DOWN:         *key = MEGADUCK_KBD_CODE_ARROW_DOWN;  return;
            case SDL_SCANCODE_KP_MINUS:     *key = MEGADUCK_KBD_CODE_MINUS;       return;

            // Continued Row 3
            case SDL_SCANCODE_LEFT:         *key = MEGADUCK_KBD_CODE_ARROW_LEFT;  return;
            case SDL_SCANCODE_KP_EQUALS:    *key = MEGADUCK_KBD_CODE_EQUALS;      return;
            case SDL_SCANCODE_RIGHT:        *key = MEGADUCK_KBD_CODE_ARROW_RIGHT; return;

            // Continued Row 2
            case SDL_SCANCODE_KP_DIVIDE:    *key = MEGADUCK_KBD_CODE_DIVIDE;      return;
            case SDL_SCANCODE_UP:           *key = MEGADUCK_KBD_CODE_ARROW_UP;    return;
            case SDL_SCANCODE_KP_PLUS:      *key = MEGADUCK_KBD_CODE_PLUS;        return;

            case SDL_SCANCODE_PRINTSCREEN: *key = MEGADUCK_KBD_CODE_PRINTSCREEN_RIGHT; return;
            case SDL_SCANCODE_SCROLLLOCK:  *key = MEGADUCK_KBD_CODE_PRINTSCREEN_RIGHT; return;

            default: return;
        } // switch (event.key.keysym.scancode)

    } // else if ((event.type == SDL_KEYDOWN) || (event.type == SDL_KEYUP))

};