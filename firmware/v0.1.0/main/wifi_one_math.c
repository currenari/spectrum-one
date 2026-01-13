/**
 * @file wifi_one_math.c
 * @brief Helper conversions and LED mapping for Spectrum One.
 *
 * Provides:
 * - dBm to mW conversion
 * - Total mW to field estimate (nW/m2) using a simple heuristic
 * - Mapping functions to convert values into 0..10 LED levels
 *
 * Outputs are derived from WiFi RSSI and are not calibrated measurements.
 */

#include "wifi_one_math.h"

#include <math.h>
#include <stdbool.h>

static inline bool is_bad_float(float x)
{
    return isnan(x) || isinf(x);
}

float wifi_one_mw_from_dbm(float dbm)
{
    if (is_bad_float(dbm) || dbm <= -120.0f) {
        return 0.0f;
    }

    return powf(10.0f, dbm / 10.0f);
}

/*
 * Field estimate from total power.
 *
 * Converts summed mW to nW, then divides by a fixed effective area.
 * This is a heuristic used for consistent on-device visualisation.
 *
 * Assumed area: 4 cm2 (0.0004 m2)
 */
float wifi_one_field_from_mw(float total_mw)
{
    if (is_bad_float(total_mw) || total_mw <= 0.0f) {
        return 0.0f;
    }

    const float area_m2  = 0.0004f;
    const float mw_to_nw = 1e6f;

    float density_nw_m2 = (total_mw * mw_to_nw) / area_m2;

    if (is_bad_float(density_nw_m2) || density_nw_m2 < 0.0f) {
        return 0.0f;
    }

    return density_nw_m2;
}

/*
 * Field estimate (nW/m2) to 10-segment LED level (0..10).
 * Thresholds are project-tuned for stable visual response.
 */
int wifi_one_led_from_field(float field_nw)
{
    if (is_bad_float(field_nw) || field_nw <= 0.0f) {
        return 0;
    }

    if (field_nw <      100.0f) return 1;
    if (field_nw <      500.0f) return 2;
    if (field_nw <     5000.0f) return 3;
    if (field_nw <    20000.0f) return 4;
    if (field_nw <    50000.0f) return 5;
    if (field_nw <   250000.0f) return 6;
    if (field_nw <   500000.0f) return 7;
    if (field_nw <  1000000.0f) return 8;
    if (field_nw <  3000000.0f) return 9;

    return 10;
}

int wifi_one_led_from_field_mw(float total_mw)
{
    return wifi_one_led_from_field(wifi_one_field_from_mw(total_mw));
}

/*
 * RSSI (dBm) to 10-segment LED level (0..10).
 * Linear mapping between rssi_min and rssi_max.
 */
int wifi_one_led_from_ap_rssi(int rssi_dbm)
{
    const int rssi_min = -95;
    const int rssi_max = -35;

    if (rssi_dbm <= rssi_min) {
        return 0;
    }
    if (rssi_dbm >= rssi_max) {
        return 10;
    }

    float span = (float)(rssi_max - rssi_min);
    float norm = (float)(rssi_dbm - rssi_min) / span;

    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;

    int band = (int)(norm * 10.0f);

    if (band < 0) band = 0;
    if (band > 9) band = 9;

    return band + 1;
}

/*
 * Total power (dBm) to 10-segment LED level (0..10).
 * Linear mapping between min_dbm and max_dbm.
 */
int wifi_one_led_from_total_dbm(float total_dbm)
{
    if (is_bad_float(total_dbm)) {
        return 0;
    }

    const float min_dbm = -95.0f;
    const float max_dbm = -30.0f;

    if (total_dbm <= min_dbm) {
        return 0;
    }
    if (total_dbm >= max_dbm) {
        return 10;
    }

    float span = max_dbm - min_dbm;
    float norm = (total_dbm - min_dbm) / span;

    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;

    int band = (int)(norm * 10.0f);

    if (band < 0) band = 0;
    if (band > 9) band = 9;

    return band + 1;
}
