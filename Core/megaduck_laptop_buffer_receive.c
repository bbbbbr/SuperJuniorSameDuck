#include "gb.h"
#include <time.h>
#include <string.h>
#include <stdio.h>

#include "megaduck_laptop_periph.h"



void MD_receive_buf_init(GB_megaduck_laptop_t * periph) {

    // Save request type as purpose of RX Buffer
    periph->rx_buffer_state    = MEGADUCK_RX_BUF_1_LEN;
    periph->rx_buffer_purpose  = periph->byte_being_received;
    periph->rx_buffer_checksum = 0;
    periph->rx_buffer_size     = 0;
    periph->rx_buffer_count    = 0;

    MD_send_buf_enqueue(periph, MEGADUCK_SYS_REPLY_SEND_BUFFER_OK);
    MD_send_buf_finalize_and_transmit(periph);
    // Override with longer delay for first reply byte
    // since command that initiated the buffer requires 2+ msec delay
    // for unknown reasons (maybe extra delay for RTC latch on reads?)
    periph->t_states_till_update = MEGADUCK_LAPTOP_TICK_COUNT_RX_BUF_START;
}


void MD_receive_buf_handle_result(GB_megaduck_laptop_t * periph) {

    switch (periph->rx_buffer_purpose) {
        case MEGADUCK_SYS_CMD_RTC_SET_DATE_AND_TIME:
            MD_rtc_set_from_buf(periph);
            break;

        case MEGADUCK_SYS_CMD_PLAYSPEECH:
            MD_speech_playback(periph);
            break;

        case MEGADUCK_SYS_CMD_PRINT_SEND_BYTES:
            MD_printer_process_buf(periph);
            break;
    }
}


void MD_receive_buf(GB_megaduck_laptop_t * periph) {

    switch (periph->rx_buffer_state) {

        case MEGADUCK_RX_BUF_1_LEN:
            periph->rx_buffer_size = (int)periph->byte_being_received - 2; // -1 for size byte and -1 for checksum byte
            periph->rx_buffer_count = 0; // 0 bytes of payload received so far
            if (periph->rx_buffer_size > 0) {
                periph->rx_buffer_state = MEGADUCK_RX_BUF_2_PAYLOAD;
                MD_send_buf_enqueue(periph, MEGADUCK_SYS_REPLY_SEND_BUFFER_OK);
            } else {
                periph->rx_buffer_state = MEGADUCK_RX_BUF_5_FAIL;
                MD_send_buf_enqueue(periph, MEGADUCK_SYS_REPLY_SEND_BUFFER_MAYBE_ERROR); // Implied by System ROM but not verified on hardware
            }
            #ifdef MEGADUCK_SYS_SERIAL_LOG_RX_BUFFER
                printf("*** RX BUFFER: size=%d (raw:0x%02x)\n", periph->rx_buffer_size, periph->rx_buffer_size + 2);
            #endif
            MD_send_buf_finalize_and_transmit(periph);
            break;

        case MEGADUCK_RX_BUF_2_PAYLOAD:
            // #ifdef MEGADUCK_SYS_SERIAL_LOG_RX_BUFFER
            //     printf("*** RX BUFFER: [%d] = 0x%02x\n", periph->rx_buffer_count, periph->byte_being_received);
            // #endif

            // Save received data byte, increment counter
            if (periph->rx_buffer_count < MEGADUCK_BUF_SZ)
                periph->rx_buffer[periph->rx_buffer_count] = periph->byte_being_received;

            // If that was the last byte, switch to checksum mode for next incoming byte
            periph->rx_buffer_count++;
            if (periph->rx_buffer_count == periph->rx_buffer_size)
                periph->rx_buffer_state = MEGADUCK_RX_BUF_3_CHECKSUM;

            MD_send_buf_enqueue(periph, MEGADUCK_SYS_REPLY_SEND_BUFFER_OK);
            MD_send_buf_finalize_and_transmit(periph);
            break;

        case MEGADUCK_RX_BUF_3_CHECKSUM:
            // Should be last byte received in the sequence
            periph->rx_buffer_checksum = periph->byte_being_received;

            // Initialize checksum to length (Buffer Size + 2)
            // Then add all payload bytes
            uint8_t checksum_calc = (uint8_t)periph->rx_buffer_size + 2; // +1 for size byte and +1 for checksum byte
            for (int c = 0; c < periph->rx_buffer_size; c++) {
                #ifdef MEGADUCK_SYS_SERIAL_LOG_RX_BUFFER
                    printf("rx buf [%d] = 0x%02x\n", c, periph->rx_buffer[c]);
                #endif
                checksum_calc += periph->rx_buffer[c];
            }
            #ifdef MEGADUCK_SYS_SERIAL_LOG_RX_BUFFER
                printf("rx buf checksum = 0x%02x, calc_checksum 2's comp = 0x%02x, calc_checksum raw = 0x%02x, size = %d\n",
                    periph->rx_buffer_checksum, (uint8_t)(~checksum_calc + 1), checksum_calc, periph->rx_buffer_size);
            #endif

            // Rx Checksum Byte should == (((sum of all bytes except checksum) XOR 0xFF) + 1) [two's complement]
            // so ((sum of received bytes including checksum byte) should == -> unsigned 8 bit overflow -> 0x00
            checksum_calc += periph->rx_buffer_checksum;
            if (checksum_calc == 0) {
                // Success
                MD_receive_buf_handle_result(periph);
                periph->rx_buffer_state = MEGADUCK_RX_BUF_4_DONE;
                MD_send_buf_enqueue(periph, MEGADUCK_SYS_REPLY_BUFFER_SEND_AND_CHECKSUM_OK);
            } else {
                periph->rx_buffer_state = MEGADUCK_RX_BUF_5_FAIL;
                MD_send_buf_enqueue(periph, MEGADUCK_SYS_REPLY_BUFFER_SEND_AND_CHECKSUM_FAIL); // Implied by System ROM but not verified on hardware
            }
            MD_send_buf_finalize_and_transmit(periph);
            break;
    }  // end: switch (periph->rx_buffer_state)
}

