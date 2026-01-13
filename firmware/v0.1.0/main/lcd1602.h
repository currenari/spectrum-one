#ifndef LCD1602_H
#define LCD1602_H

#include <stdint.h>

/* C linkage when included from C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialise the LCD and I2C interface.
 */
void lcd_init(void);

/**
 * Power-on welcome sequence shown at startup.
 */
void lcd_show_welcome(void);

/**
 * Display scan result or selected access point.
 *
 * Line 1: RSSI <x>dBm AP<y>
 * Line 2: SSID <name> (clipped to fit)
 */
void lcd_show_scan_result(int best_rssi, uint16_t ap_num, const char *ssid);

/**
 * Display total power estimate.
 *
 * Line 1: SUM <dBm>
 * Line 2: PWR <mW>
 */
void lcd_show_total_power(float total_dbm, float total_mw);

/**
 * Display approximate field level.
 *
 * Line 1: Field Level
 * Line 2: Value with unit scaling (nW/m2)
 *
 * @param field_nw Field strength in nW/m2.
 */
void lcd_show_field_level(float field_nw);

/**
 * Display a simple two-line status screen.
 *
 * Each line is clipped to 16 characters.
 * NULL pointers are treated as empty lines.
 */
void lcd_show_status(const char *line1, const char *line2);

#ifdef __cplusplus
}
#endif

#endif /* LCD1602_H */
