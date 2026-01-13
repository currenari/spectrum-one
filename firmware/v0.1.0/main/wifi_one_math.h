#ifndef WIFI_ONE_MATH_H
#define WIFI_ONE_MATH_H

#include <stdint.h>

/* C linkage when included from C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Convert total mW estimate into a field-style value (nW/m2).
 *
 * Heuristic used for the Spectrum One UI.
 * Derived from WiFi RSSI and not a calibrated measurement.
 */
float wifi_one_field_from_mw(float total_mw);

/**
 * Map field estimate (nW/m2) to a 0..10 LED level.
 */
int wifi_one_led_from_field(float field_nw);

/**
 * Convenience wrapper: mW -> field -> LED level (0..10).
 */
int wifi_one_led_from_field_mw(float total_mw);

/**
 * Map a single AP RSSI (dBm) to a 0..10 LED level.
 */
int wifi_one_led_from_ap_rssi(int rssi_dbm);

/**
 * Map total dBm estimate to a 0..10 LED level.
 */
int wifi_one_led_from_total_dbm(float total_dbm);

/**
 * Convert dBm to mW.
 */
float wifi_one_mw_from_dbm(float dbm);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_ONE_MATH_H */
