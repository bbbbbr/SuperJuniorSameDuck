#include "gb.h"
#include <time.h>
#include <string.h>
#include <stdio.h>

// TODO: maybe reset ext clock send bit counter during new transmits? maybe more accurate not to. periph->ext_clk_send_bit_counter = 0;

#include "megaduck_laptop_periph.h"



// Serial clock speed used is: 8192 Hz  1 KB/s  Bit 1 cleared, Normal speed
//
// 8192 Hz / 8 bits = 1024 Bytes per second / 1000 msec = 1.024 msec per byte


void MD_periph_reset(GB_megaduck_laptop_t * periph, uint8_t target_state) {
    periph->state                    = MEGADUCK_SYS_STATE_INIT_1_WAIT_RX_COUNTER;
    periph->init_counter             = MEGADUCK_SYS_INIT_COUNTER_RESET;
    periph->ext_clk_send_bit_counter = 0;
    periph->ext_clk_send_buf_size  = 0;
    periph->ext_clk_send_buf_index = 0;

    periph->key           = MEGADUCK_KBD_CODE_NONE;
    periph->key_modifiers = MEGADUCK_KBD_FLAGS_NONE;
}


static void idle_handle_commands(GB_gameboy_t *gb, GB_megaduck_laptop_t * periph) {

    #ifdef DEBUG_LOG_DUCK_SERIAL_RX_IDLE_CMD
        GB_log(gb, "\n-> [idle] rx cmd = 0x%0X\n", periph->byte_being_received);
    #endif

    switch (periph->byte_being_received) {
        case MEGADUCK_SYS_CMD_PRINT_INIT_MAYBE_EXT_IO:
            periph->state = MEGADUCK_SYS_STATE_REPLY_CMD_PRINT_INIT_MAYBE_EXT_IO;
            // TODO Bit.1 indicates printer type (1 = single pass large buffer, 0 = two pass small buffer)
            // So maybe 0x01 = 2 pass printing, 0x03 = 1 pass printing
            MD_send_buf_enqueue(periph, MD_printer_init(periph)); // MEGADUCK_SYS_REPLY_CMD_INIT_UNKNOWN_0x09
            MD_send_buf_finalize_and_transmit(periph);
            break;

        case MEGADUCK_SYS_CMD_GET_KEYS:
            periph->state = MEGADUCK_SYS_STATE_GET_KEYS_TX;
            MD_keyboard_enqueue_reply(periph);
            break;

        case MEGADUCK_SYS_CMD_RTC_GET_DATE_AND_TIME:
            periph->state = MEGADUCK_SYS_STATE_GET_RTC_TX;
            MD_rtc_enqueue_reply(periph);
            break;

        case MEGADUCK_SYS_CMD_RTC_SET_DATE_AND_TIME:
            periph->state = MEGADUCK_SYS_STATE_CMD_SET_RTC;
            MD_receive_buf_init(periph);
            break;

        case MEGADUCK_SYS_CMD_PRINT_SEND_BYTES:
            periph->state = MEGADUCK_SYS_STATE_CMD_PRINT_SEND_BYTES;
            MD_receive_buf_init(periph);
            break;

        case MEGADUCK_SYS_CMD_PLAYSPEECH:
            periph->state = MEGADUCK_SYS_STATE_CMD_PLAYSPEECH;
            MD_receive_buf_init(periph);
            break;

        default:
            printf(" !! Duck Serial Periph: Unknown command received during idle+configured state: 0x%02x (may not be sio corrected, so read as >> 1)\n", periph->byte_being_received);
            break;
    }
    // TX/Reply byte is ignored by System ROM
    periph->byte_to_send = 0x00; // TODO: Also not yet known what value it typically is
}


// From the perspective of the Peripheral hardware
static void handle_received_byte(GB_gameboy_t *gb, GB_megaduck_laptop_t * periph) {

    switch(periph->state) {
        case MEGADUCK_SYS_STATE_INIT_1_WAIT_RX_COUNTER:
            MD_init_stage_1_rx_counter(periph);
            break;

        case MEGADUCK_SYS_STATE_INIT_2_SEND_RX_COUNT_ACK:
            // Do nothing here, handled by periph_megaduck_laptop_peripheral_update()
            break;

        case MEGADUCK_SYS_STATE_INIT_3_WAIT_TX_COUNTER_REQ:
            MD_init_stage_3_wait_tx_count_request(periph);
            break;

        case MEGADUCK_SYS_STATE_INIT_4_SEND_TX_COUNTER:
             // Fall through, System ROM might try to interrupt TX counter at any time in sequence if it fails
             // even though it looks like that might result in both devices trying to drive the clock
        case MEGADUCK_SYS_STATE_INIT_5_WAIT_TX_COUNT_ACK:
            MD_init_stage_5_wait_tx_count_ack(periph);
            break;

        // Once Initialized handles commands sent from the MegaDuck
        case MEGADUCK_SYS_STATE_INIT_OK_READY:
            idle_handle_commands(gb, periph);
            break;

        // Multi-Byte Receive *FROM* the MegaDuck
        case MEGADUCK_SYS_STATE_CMD_PLAYSPEECH:  // Fall through to shared handling
        case MEGADUCK_SYS_STATE_CMD_SET_RTC:
        case MEGADUCK_SYS_STATE_CMD_PRINT_SEND_BYTES:
            // Replies handled via periph_megaduck_laptop_peripheral_update()
            MD_receive_buf(periph);
            break;

        // Multi-Byte Send *TO* the MegaDuck
        case MEGADUCK_SYS_STATE_GET_RTC_TX:
        case MEGADUCK_SYS_STATE_GET_KEYS_TX:
             // Fall through, System ROM might try to interrupt keyboard reply at any time in sequence if it fails
        case MEGADUCK_SYS_STATE_GET_KEYS_WAIT_ACK:
        case MEGADUCK_SYS_STATE_GET_RTC_WAIT_ACK:
            MD_send_buf_handle_tx_reply(periph);
            break;
    }
}

// Serial bit clocking Pre-handler
static void serial_start(GB_gameboy_t *gb, bool bit_received) {

    GB_megaduck_laptop_t * periph = &gb->megaduck_laptop;

    #ifdef DEBUG_LOG_DUCK_SERIAL_IO_BITS
        GB_log(gb, "\n-> PERIPH: serial_start: bit_rx=%d[sio_ct: %d] | div=0x%04x | serial_master_clock=%d\n", bit_received, gb->serial_count, gb->div_counter, gb->serial_master_clock);
    #endif
    periph->byte_being_received <<= 1;
    periph->byte_being_received |= bit_received;
    periph->bits_received++;
    if (periph->bits_received == 8) {

        #ifdef MEGADUCK_SYS_SERIAL_LOGGING_ENABLED
            #ifdef MEGADUCK_SYS_SERIAL_LOG_ALL_IN_OUT
                GB_log(gb, "\n--> Serial to PERIPH: 0x%02x  [State: %d, RX Counter: %d, EXT-queue: %d]\n",
                    periph->byte_being_received,
                    periph->state, periph->init_counter, periph->ext_clk_send_buf_size);

                // GB_log(gb, "->> Periph:  from-gb-RX:%02x  ( ROM0/B:%02x %02x  PC:%04x ) [State: %d, RX Counter: %d, EXT-queue: %d]\n",
                //     periph->byte_being_received, // periph->byte_being_received,
                //     // periph->byte_to_send, periph->byte_to_send,
                //      gb->mbc_rom0_bank, gb->mbc_rom_bank, gb->pc,
                //      periph->state, periph->init_counter, periph->ext_clk_send_buf_size);
            #endif
        #endif

        handle_received_byte(gb, periph);

        #ifdef MEGADUCK_SYS_SERIAL_LOGGING_ENABLED
            #ifdef MEGADUCK_SYS_SERIAL_LOG_ALL_IN_OUT
                GB_log(gb, "    - (post serial handling)  [State: %d, RX Counter: %d, EXT-queue: %d, Ticks: %d]\n",
                     periph->state, periph->init_counter, periph->ext_clk_send_buf_size,
                                periph->t_states_till_update);
            #endif
        #endif


        periph->bits_received = 0;
        periph->byte_being_received = 0;
    }  // End: 8 bits received
}


// Serial bit clocking Post-handler
static bool serial_end(GB_gameboy_t *gb) {
    bool ret = gb->megaduck_laptop.bit_to_send;
    gb->megaduck_laptop.bit_to_send = gb->megaduck_laptop.byte_to_send & 0x80;
    gb->megaduck_laptop.byte_to_send <<= 1;
    return ret;
}


void GB_connect_megaduck_laptop(GB_gameboy_t *gb,
                                GB_megaduck_laptop_set_time_callback set_time_callback,
                                GB_megaduck_laptop_get_time_callback get_time_callback)
{
    memset(&gb->megaduck_laptop, 0, sizeof(gb->megaduck_laptop));
    GB_set_serial_transfer_bit_start_callback(gb, serial_start);
    GB_set_serial_transfer_bit_end_callback(gb, serial_end);
    gb->megaduck_laptop_set_time_callback = set_time_callback;
    gb->megaduck_laptop_get_time_callback = get_time_callback;
    gb->accessory = GB_ACCESSORY_MEGADUCK_LAPTOP;

    MD_periph_reset(&gb->megaduck_laptop, MEGADUCK_SYS_POWER_ON_RESET);
}

void GB_megaduck_laptop_use_alt_initial_stack_value(GB_gameboy_t *gb) {
    gb->use_megaduck_laptop_initial_sp = true;
}


bool GB_megaduck_laptop_is_enabled(GB_gameboy_t *gb) {
    return gb->accessory == GB_ACCESSORY_MEGADUCK_LAPTOP; // TODO:  && gb->megaduck_laptop.mode;
}


void GB_megaduck_laptop_key_set(GB_gameboy_t *gb, uint8_t key, uint8_t key_modifiers) {

    if (gb->accessory != GB_ACCESSORY_MEGADUCK_LAPTOP) return;
    // GB_log(gb, "key=0x%02x, key_mod=0x%02x (Prev: key=0x%02x, key_mod=0x%02x) VS (Prev: key=0x%02x, key_mod=0x%02x)\n",
    //        key, key_modifiers, gb->megaduck_laptop.key, gb->megaduck_laptop.key_modifiers, gb->megaduck_laptop.last_key, gb->megaduck_laptop.last_key_modifiers);

    gb->megaduck_laptop.key = key;
    gb->megaduck_laptop.key_modifiers = key_modifiers;
}


void GB_megaduck_laptop_key_release(GB_gameboy_t *gb, uint8_t key, uint8_t key_modifiers) {
    if (gb->accessory != GB_ACCESSORY_MEGADUCK_LAPTOP) return;

    // TODO: To make this more accurate vs Hardware: track and maintain state of all "hardware keys" pressed
    //       instead of releasing *all* keys when any single key is released.
    //       Then when sending which single key is pressed, transmit the one with the highest value
    //       (roughly how it seems to work, though there may be some odd exceptions to that
    //        which lock up sending any key if they are pressed as a secondary or more key?).

    // GB_log(gb, "released: key=0x%02x, key_mod=0x%02x (Prev: key=0x%02x, key_mod=0x%02x) VS (Prev: key=0x%02x, key_mod=0x%02x)\n",
    //        key, key_modifiers, gb->megaduck_laptop.key, gb->megaduck_laptop.key_modifiers, gb->megaduck_laptop.last_key, gb->megaduck_laptop.last_key_modifiers);

    // Always clear the key on keyup events
    // regardless if a match was found in PC -> Duck key translation
    // if (key != MEGADUCK_KBD_CODE_NONE)
        gb->megaduck_laptop.key = MEGADUCK_KBD_CODE_NONE;

    // No need to clear key repeat flag since key_modifiers argument to this function will never have it set
    gb->megaduck_laptop.key_modifiers = key_modifiers;

    // Update key repeat tracking
    gb->megaduck_laptop.last_key = gb->megaduck_laptop.key;
    gb->megaduck_laptop.last_key_modifiers = gb->megaduck_laptop.key_modifiers;
}


void GB_megaduck_laptop_reset(GB_gameboy_t *gb) {
    MD_periph_reset(&gb->megaduck_laptop, MEGADUCK_SYS_POWER_ON_RESET);
}


// Called from timing.h after 512 T-States have elapsed
// Roughly matches serial clock timing in GB "normal" speed
//
// For sending serial data from peripheral when GB is waiting
// for data to be initiated via external clock
//
// "cycles" param here in SameBoy looks like it means 4.n MHz T-States
void GB_megaduck_laptop_peripheral_update(GB_gameboy_t *gb, uint8_t cycles) {
    GB_megaduck_laptop_t * periph = &gb->megaduck_laptop;

    // GB_log(gb, "  megaduck_laptop [EXT CLK] --------TICK---------\n");
    // Reset counter until next update
    periph->t_states_till_update = MEGADUCK_LAPTOP_TICK_COUNT_RESET;

    if (periph->ext_clk_send_buf_size > 0) {

        // Send data 1 bit at a time
        bool tx_bit = (periph->ext_clk_send_buf[ periph->ext_clk_send_buf_index ] & 0x80) == 0x80;

        // GB_log(gb, "  megaduck_laptop [EXT CLK] ** BIT SEND ** %d = %d (%02x) [queued: %d] {DIV:%04x, PC=0x%04x, SC=0x%02x}\n",
        //     periph->ext_clk_send_bit_counter, tx_bit, periph->ext_clk_send_buf[ periph->ext_clk_send_buf_index ],
        //         periph->ext_clk_send_buf_size,
        //         gb->div_counter, gb->pc, gb->io_registers[GB_IO_SC]);

        if (periph->ext_clk_send_buf_index <= MEGADUCK_BUF_SZ)
            periph->ext_clk_send_buf[ periph->ext_clk_send_buf_index ] <<= 1;

        GB_serial_set_data_bit(gb, tx_bit);
        // TODO: Duck laptop peripheral doesn't seem to care about RX data when it's driving the clock
        // GB_serial_get_data_bit(GB_gameboy_t *gb);

        // Once 8 bits sent, move to next byte in queue, or if done change state
        periph->ext_clk_send_bit_counter++;
        if (periph->ext_clk_send_bit_counter == 8) {

            periph->ext_clk_send_bit_counter = 0;

            // #ifdef MEGADUCK_SYS_SERIAL_LOGGING_ENABLED
            //     #ifdef MEGADUCK_SYS_SERIAL_LOG_ALL_IN_OUT
            //         GB_log(gb, "    - megaduck_laptop [EXT CLK] send byte done, queue size %d -> %d   ( ROM0/B:%02x %02x  PC:%04x ) [State: %d, RX Counter: %d, EXT-queue: %d]\n",
            //             periph->ext_clk_send_buf_size, periph->ext_clk_send_buf_size - 1,
            //              gb->mbc_rom0_bank, gb->mbc_rom_bank, gb->pc,
            //              periph->state, periph->init_counter, periph->ext_clk_send_buf_size);
            //     #endif
            // #endif

            // Move to next byte
            periph->ext_clk_send_buf_size--;
            if (periph->ext_clk_send_buf_size == 0) {

                // If send queue is done handle changes to next state
                switch(periph->state) {

                    case MEGADUCK_SYS_STATE_INIT_2_SEND_RX_COUNT_ACK:
                        periph->state = MEGADUCK_SYS_STATE_INIT_3_WAIT_TX_COUNTER_REQ;
                        break;

                    case MEGADUCK_SYS_STATE_INIT_4_SEND_TX_COUNTER:
                        periph->state = MEGADUCK_SYS_STATE_INIT_5_WAIT_TX_COUNT_ACK;
                        break;

                    case MEGADUCK_SYS_STATE_REPLY_CMD_PRINT_INIT_MAYBE_EXT_IO:
                        // Return to initialized ready waiting state
                        periph->state = MEGADUCK_SYS_STATE_INIT_OK_READY;
                        break;

                    case MEGADUCK_SYS_STATE_CMD_PLAYSPEECH: // Fall through to shared handling
                    case MEGADUCK_SYS_STATE_CMD_SET_RTC:
                    case MEGADUCK_SYS_STATE_CMD_PRINT_SEND_BYTES:
                        // Multi-byte buffer receive
                        if ((periph->rx_buffer_state == MEGADUCK_RX_BUF_4_DONE) ||
                            (periph->rx_buffer_state == MEGADUCK_RX_BUF_5_FAIL)) {
                            // Return to initialized ready waiting state
                            periph->state = MEGADUCK_SYS_STATE_INIT_OK_READY;
                        }
                        break;

                    case MEGADUCK_SYS_STATE_GET_KEYS_TX:
                        // Last byte sent, now wait for acknowledgement
                        periph->state = MEGADUCK_SYS_STATE_GET_KEYS_WAIT_ACK;
                        break;

                    case MEGADUCK_SYS_STATE_GET_RTC_TX:
                        // Last byte sent, now wait for acknowledgement
                        periph->state = MEGADUCK_SYS_STATE_GET_RTC_WAIT_ACK;
                        break;

                }
            } else {
                // If there is another byte to send, reset and add a longer delay until next serial bit send
                periph->ext_clk_send_buf_index++;  // TODO: could reset this inside ==0 block above once whole send buffer is done

                if ((periph->state == MEGADUCK_SYS_STATE_GET_KEYS_TX) ||
                    (periph->state == MEGADUCK_SYS_STATE_GET_RTC_TX)) {  // TODO: make this more generic (i.e. .is_sending_buffer)
                    // Keyboard reply needs a longer delay between transmits
                    periph->t_states_till_update = MEGADUCK_LAPTOP_TICK_COUNT_TX_BUF_REPLY_NEXT;
                } else {
                    periph->t_states_till_update = MEGADUCK_LAPTOP_TICK_COUNT_TX_DELAY;
                }
            }

            #ifdef MEGADUCK_SYS_SERIAL_LOGGING_ENABLED
                #ifdef MEGADUCK_SYS_SERIAL_LOG_ALL_IN_OUT
                    // GB_log(gb, "    - megaduck_laptop [EXT CLK] send byte done, queue size %d -> %d   ( ROM0/B:%02x %02x  PC:%04x ) [State: %d, RX Counter: %d, EXT-queue: %d]\n",
                        // periph->ext_clk_send_buf_size + 1, periph->ext_clk_send_buf_size,
                        //  gb->mbc_rom0_bank, gb->mbc_rom_bank, gb->pc,
                        //  periph->state, periph->init_counter, periph->ext_clk_send_buf_size);
                    GB_log(gb, "    - send done, queue %d -> %d  [State: %d, RX Counter: %d, , Ticks: %d]\n",
                        periph->ext_clk_send_buf_size + 1, periph->ext_clk_send_buf_size,
                        periph->state, periph->init_counter, periph->t_states_till_update);
                #endif
            #endif

        } // if (periph->ext_clk_send_bit_counter == 8)
    } // if (periph->ext_clk_send_buf_size > 0)

}

