/**
 * @file wifi_scan.h
 *
 * WiFi scan API and result structures for Spectrum One.
 *
 * wifi_scan_init()
 *   Initialises NVS, network stack, and WiFi in STA mode.
 *
 * wifi_scan_run()
 *   Runs a blocking scan and fills wifi_scan_result_t with:
 *   - Access point list (capped)
 *   - Strongest access point summary
 *   - Total power estimate (mW and dBm)
 */

#ifndef WIFI_SCAN_H
#define WIFI_SCAN_H

#include <stdint.h>
#include <stdbool.h>

/* C linkage when included from C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Maximum number of access points copied from a scan.
 * The scan may return more, but the stored list is capped for fixed memory use.
 */
#define WIFI_SCAN_MAX_APS 20

typedef struct {
    int16_t rssi;      /* RSSI in dBm */
    uint8_t primary;   /* WiFi channel */
    char    ssid[33];  /* SSID (32 max) + terminator */
} wifi_ap_info_t;

typedef struct {
    uint16_t       ap_count;                       /* 0..WIFI_SCAN_MAX_APS */
    wifi_ap_info_t ap_records[WIFI_SCAN_MAX_APS];

    int            best_rssi;
    char           best_ssid[33];

    float          total_dbm;
    float          total_mw;
} wifi_scan_result_t;

/**
 * Initialise WiFi in STA mode for scanning.
 * Call once before wifi_scan_run().
 */
void wifi_scan_init(void);

/**
 * Run a blocking WiFi scan and compute derived values.
 *
 * @param out Result structure to fill.
 * @return true on a completed scan (including empty results).
 * @return false when @p out is NULL.
 */
bool wifi_scan_run(wifi_scan_result_t *out);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_SCAN_H */
