#include "gb.h"
#include <time.h>
#include <string.h>
#include <stdio.h>

#include "megaduck_laptop_periph.h"

// TODO: Consider ignoring the automatic RTC reset behavior 
//       of the Spanish System ROM when it detects the rtc valid sequence
//       at WRAM 0xDBFC is not present.
//
// * Spanish System ROM
//   Year  = 94 <- WRITE rtc TO periph
//   Month = 01
//   Day   = 01
//   DoW   = 06
//   Hour  = 00
//   AM/PM = 00
//   Min   = 00
//   Sec   = 00



#ifdef _WIN32
    #define timegm _mkgmtime
#endif

static uint8_t int_to_bcd(uint8_t i);
static uint8_t bcd_to_int(uint8_t i);

static uint8_t int_to_bcd(uint8_t i)
{
    return (i % 10) + ((i / 10) << 4);
}

static uint8_t bcd_to_int(uint8_t i)
{
    return (i & 0xF) + (i >> 4) * 10;
}


// For Write to peripheral RTC (from Duck ROM perspective)
void MD_rtc_set_from_buf(GB_megaduck_laptop_t * periph) {

    if (periph->rx_buffer_size == MEGADUCK_BUF_SZ_RTC) {
        // printf("MD_rtc_set_from_buf()\n");

        struct tm tm_rtc_time = {0};

        // tm_year is in format N + 1900
        int rtc_year = bcd_to_int(periph->rx_buffer[MEGADUCK_RTC_IDX_YEAR]);
        if (rtc_year < 92) rtc_year += 100; // Shift to 2000's.       0x00 -> 0x91 = 2000 -> 2091
                                            // Otherwise Leave as is. 0x92 -> 0x99 = 1992 -> 1999

        tm_rtc_time.tm_year = rtc_year; // In BCD with a rollover to 00 at 2000. System ROM uses dates as old as 1992 (0x92)
        tm_rtc_time.tm_mon  = bcd_to_int(periph->rx_buffer[MEGADUCK_RTC_IDX_MON]) - 1;
        tm_rtc_time.tm_mday = bcd_to_int(periph->rx_buffer[MEGADUCK_RTC_IDX_DAY]);
        // MEGADUCK_RTC_IDX_DOW (ignore this, let timegm() recalculate it)
        tm_rtc_time.tm_hour = bcd_to_int(periph->rx_buffer[MEGADUCK_RTC_IDX_HOUR])
                                              + (periph->rx_buffer[MEGADUCK_RTC_IDX_AMPM] * 12);
        tm_rtc_time.tm_min  = bcd_to_int(periph->rx_buffer[MEGADUCK_RTC_IDX_MIN]);
        tm_rtc_time.tm_sec  = bcd_to_int(periph->rx_buffer[MEGADUCK_RTC_IDX_SEC]);

        #ifdef DEBUG_LOG_DUCK_RTC_COMMANDS_DATA
            printf("* Year  = %02x <- WRITE rtc TO periph\n"
                   "  Month = %02x\n"
                   "  Day   = %02x\n"
                   "  DoW   = %02x\n"
                   "  Hour  = %02x\n"
                   "  AM/PM = %02x\n"
                   "  Min   = %02x\n"
                   "  Sec   = %02x\n\n",
                periph->rx_buffer[MEGADUCK_RTC_IDX_YEAR],
                periph->rx_buffer[MEGADUCK_RTC_IDX_MON],
                periph->rx_buffer[MEGADUCK_RTC_IDX_DAY],
                periph->rx_buffer[MEGADUCK_RTC_IDX_DOW],
                periph->rx_buffer[MEGADUCK_RTC_IDX_HOUR],
                periph->rx_buffer[MEGADUCK_RTC_IDX_AMPM],
                periph->rx_buffer[MEGADUCK_RTC_IDX_MIN],
                periph->rx_buffer[MEGADUCK_RTC_IDX_SEC]);
        #endif

        // Use timegm() instead mktime() to avoid having to make sure the
        // .tm_isdst (daylight savings flag) is set correctly (otherwise mktime() may add an hour)
        periph->time_delta_rtc_vs_host = time(NULL) - timegm(&tm_rtc_time);
    }
}


// Get RTC command reply (Peripheral -> Duck)
//     All values are in BCD format
//         Ex: Month = December = 12th month = 0x12 (NOT 0x0C)
// [0]: Length of reply (always 4)
// [1..8] RTC Data
// [9]: checksum
void MD_rtc_enqueue_reply(GB_megaduck_laptop_t * periph) {

    // RTC time ticking
    // printf("MD_rtc_enqueue_reply()\n");

    struct tm tm;

    #ifdef MEGADUCK_SYS_FORCE_LOCAL_DATE_TIME_REPLY
        // Ignore any transmitted RTC data and use local time
        time_t rawtime;
        time(&rawtime);
        tm = *localtime(&rawtime);
    #else
        // Load previously set RTC data
        time_t offset_time = time(NULL) - periph->time_delta_rtc_vs_host;

        // Use gmtime() instead localtime() to avoid having to make sure the
        // .tm_isdst (daylight savings flag) is set correctly (otherwise mktime() may add an hour)
        tm = *gmtime(&offset_time);
    #endif

    MD_send_buf_enqueue(periph, MEGADUCK_RTC_BUF_REPLY_LEN);

    if (tm.tm_year > (2000 - 1900))
        MD_send_buf_enqueue(periph, int_to_bcd(tm.tm_year - (2000 - 1900)));
    else
        MD_send_buf_enqueue(periph, int_to_bcd(tm.tm_year)); // Years in BCD since 1900 (tm_year is already in since 1900 format)
    MD_send_buf_enqueue(periph, int_to_bcd(tm.tm_mon + 1)   );
    MD_send_buf_enqueue(periph, int_to_bcd(tm.tm_mday)      );
    MD_send_buf_enqueue(periph, int_to_bcd(tm.tm_wday)      ); // DOW (tm_wday, days since Sunday, 0-6)

    MD_send_buf_enqueue(periph, int_to_bcd( (tm.tm_hour < 12) ? 0 : 1 )); // AMPM
    MD_send_buf_enqueue(periph, int_to_bcd(tm.tm_hour % 12));
    MD_send_buf_enqueue(periph, int_to_bcd(tm.tm_min)      );
    MD_send_buf_enqueue(periph, int_to_bcd(tm.tm_sec)      );

    MD_send_buf_calc_enqueue_checksum(periph);
    MD_send_buf_finalize_and_transmit(periph);

        #ifdef DEBUG_LOG_DUCK_RTC_COMMANDS_DATA
            printf("* Year  = %02x  <- READ rtc FROM periph\n"
                   "  Month = %02x\n"
                   "  Day   = %02x\n"
                   "  DoW   = %02x\n"
                   "  AM/PM = %02x\n"
                   "  Hour  = %02x\n"
                   "  Min   = %02x\n"
                   "  Sec   = %02x\n\n",
                periph->ext_clk_send_buf[MEGADUCK_RTC_IDX_YEAR + 1],
                periph->ext_clk_send_buf[MEGADUCK_RTC_IDX_MON + 1],
                periph->ext_clk_send_buf[MEGADUCK_RTC_IDX_DAY + 1],
                periph->ext_clk_send_buf[MEGADUCK_RTC_IDX_DOW + 1],
                periph->ext_clk_send_buf[MEGADUCK_RTC_IDX_AMPM + 1],
                periph->ext_clk_send_buf[MEGADUCK_RTC_IDX_HOUR + 1],
                periph->ext_clk_send_buf[MEGADUCK_RTC_IDX_MIN + 1],
                periph->ext_clk_send_buf[MEGADUCK_RTC_IDX_SEC + 1]);
        #endif

    // Override with longer delay for first reply byte
    // since command that initiated the buffer requires 2+ msec delay
    // for unknown reasons (maybe extra delay for RTC latch on reads?)
    periph->t_states_till_update = MEGADUCK_LAPTOP_TICK_COUNT_TX_BUF_REPLY_START;
}
