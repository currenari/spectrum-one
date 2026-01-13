/**
 * @file wifi_scan.c
 *
 * WiFi scanning and derived values for Spectrum One.
 *
 * Outputs:
 * - List of access points (SSID, RSSI, channel)
 * - Strongest AP (raw, or optional smoothed winner)
 * - Total power estimate (sum of per AP mW, then total dBm)
 *
 * RSSI varies from scan to scan. When two APs are close, the "strongest"
 * can swap on small changes. Optional smoothing exists for experiments.
 */

#include <string.h>
#include <math.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "wifi_scan.h"

static const char *TAG = "WIFI_SCAN";

/* -----------------------------------------------------------------------------
 * Compile-time options
 * -------------------------------------------------------------------------- */

/*
 * Option A: Smoothed strongest winner.
 * Keeps a locked winner and only switches after repeated wins with a margin.
 */
#define OPT_STRONGEST_SMOOTHED_WINNER   0

#define SWITCH_MARGIN_DB                3
#define REQUIRED_WINS                   3

/*
 * Scan timing preset.
 * Default 0,0 is the reference behaviour for this build.
 */
#define OPT_SCAN_TIME_ALT               0     /* 0 = 0,0  | 1 = 40,80 */

/* -----------------------------------------------------------------------------
 * Internal scan buffer
 * -------------------------------------------------------------------------- */

static wifi_ap_record_t aps[WIFI_SCAN_MAX_APS];

/* -----------------------------------------------------------------------------
 * Option A state (persist across scans)
 * -------------------------------------------------------------------------- */

#if OPT_STRONGEST_SMOOTHED_WINNER
static int  s_best_locked_idx     = -1;
static int  s_best_locked_rssi    = -127;
static char s_best_locked_ssid[33];
static int  s_challenger_idx      = -1;
static int  s_challenger_wins     = 0;
#endif

/* -----------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */

static void scan_time_config(wifi_scan_config_t *cfg)
{
#if OPT_SCAN_TIME_ALT
    cfg->scan_time.active.min = 40;
    cfg->scan_time.active.max = 80;
#else
    cfg->scan_time.active.min = 0;
    cfg->scan_time.active.max = 0;
#endif
}

static int find_strongest_raw(uint16_t ap_num)
{
    int best_idx  = 0;
    int best_rssi = aps[0].rssi;

    for (int i = 1; i < (int)ap_num; i++) {
        if (aps[i].rssi > best_rssi) {
            best_rssi = aps[i].rssi;
            best_idx  = i;
        }
    }
    return best_idx;
}

#if OPT_STRONGEST_SMOOTHED_WINNER
static int find_index_by_ssid(uint16_t ap_num, const char *ssid)
{
    if (ssid == NULL || ssid[0] == '\0') {
        return -1;
    }

    for (int i = 0; i < (int)ap_num; i++) {
        const char *s = (const char *)aps[i].ssid;
        if (s[0] != '\0' && strncmp(s, ssid, 32) == 0) {
            return i;
        }
    }
    return -1;
}

/*
 * Smoothed strongest:
 * - Lock initial raw strongest
 * - Only switch when a challenger beats the locked AP by SWITCH_MARGIN_DB
 *   for REQUIRED_WINS scans in a row
 */
static int pick_strongest_smoothed(uint16_t ap_num)
{
    if (ap_num == 0) {
        s_best_locked_idx     = -1;
        s_best_locked_rssi    = -127;
        s_best_locked_ssid[0] = '\0';
        s_challenger_idx      = -1;
        s_challenger_wins     = 0;
        return -1;
    }

    /* Scan order changes, refresh locked index by SSID. */
    if (s_best_locked_ssid[0] != '\0') {
        int idx = find_index_by_ssid(ap_num, s_best_locked_ssid);
        if (idx >= 0) {
            s_best_locked_idx  = idx;
            s_best_locked_rssi = aps[idx].rssi;
        } else {
            s_best_locked_idx     = -1;
            s_best_locked_rssi    = -127;
            s_best_locked_ssid[0] = '\0';
            s_challenger_idx      = -1;
            s_challenger_wins     = 0;
        }
    }

    int raw_idx  = find_strongest_raw(ap_num);
    int raw_rssi = aps[raw_idx].rssi;

    if (s_best_locked_idx < 0) {
        s_best_locked_idx  = raw_idx;
        s_best_locked_rssi = raw_rssi;

        strncpy(s_best_locked_ssid,
                (const char *)aps[raw_idx].ssid,
                sizeof(s_best_locked_ssid) - 1);
        s_best_locked_ssid[sizeof(s_best_locked_ssid) - 1] = '\0';

        s_challenger_idx  = -1;
        s_challenger_wins = 0;

        ESP_LOGI(TAG, "Strongest (smoothed): initial lock SSID=\"%s\" RSSI=%d",
                 s_best_locked_ssid, s_best_locked_rssi);
        return s_best_locked_idx;
    }

    int locked_rssi = aps[s_best_locked_idx].rssi;

    if (raw_idx == s_best_locked_idx) {
        s_challenger_idx   = -1;
        s_challenger_wins  = 0;
        s_best_locked_rssi = locked_rssi;
        return s_best_locked_idx;
    }

    if (s_challenger_idx != raw_idx) {
        s_challenger_idx  = raw_idx;
        s_challenger_wins = 0;
    }

    if (raw_rssi >= (locked_rssi + SWITCH_MARGIN_DB)) {
        s_challenger_wins++;

        ESP_LOGI(TAG,
                 "Strongest (smoothed): challenger SSID=\"%s\" RSSI=%d beats locked SSID=\"%s\" RSSI=%d wins=%d/%d",
                 (const char *)aps[raw_idx].ssid, raw_rssi,
                 s_best_locked_ssid, locked_rssi,
                 s_challenger_wins, REQUIRED_WINS);

        if (s_challenger_wins >= REQUIRED_WINS) {
            s_best_locked_idx  = raw_idx;
            s_best_locked_rssi = raw_rssi;

            strncpy(s_best_locked_ssid,
                    (const char *)aps[raw_idx].ssid,
                    sizeof(s_best_locked_ssid) - 1);
            s_best_locked_ssid[sizeof(s_best_locked_ssid) - 1] = '\0';

            s_challenger_idx  = -1;
            s_challenger_wins = 0;

            ESP_LOGI(TAG, "Strongest (smoothed): switched lock to SSID=\"%s\" RSSI=%d",
                     s_best_locked_ssid, s_best_locked_rssi);
        }
    } else {
        s_challenger_wins = 0;
    }

    return s_best_locked_idx;
}
#endif /* OPT_STRONGEST_SMOOTHED_WINNER */

/* -----------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

void wifi_scan_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {

        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, mac));
    ESP_LOGI(TAG, "STA MAC: %02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

#if OPT_STRONGEST_SMOOTHED_WINNER
    s_best_locked_idx     = -1;
    s_best_locked_rssi    = -127;
    s_best_locked_ssid[0] = '\0';
    s_challenger_idx      = -1;
    s_challenger_wins     = 0;
#endif
}

bool wifi_scan_run(wifi_scan_result_t *out)
{
    if (out == NULL) {
        ESP_LOGE(TAG, "wifi_scan_run: NULL output pointer");
        return false;
    }

    memset(out, 0, sizeof(*out));
    out->ap_count     = 0;
    out->best_rssi    = -127;
    out->best_ssid[0] = '\0';
    out->total_mw     = 0.0f;
    out->total_dbm    = -120.0f;

    wifi_scan_config_t scanConf = {
        .ssid        = NULL,
        .bssid       = NULL,
        .channel     = 0,
        .show_hidden = true,
        .scan_type   = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time   = {
            .active = { .min = 0, .max = 0 }
        }
    };

    scan_time_config(&scanConf);

    ESP_ERROR_CHECK(esp_wifi_scan_start(&scanConf, true));

    uint16_t ap_num = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_num));
    ESP_LOGI(TAG, "Scan finished, ap_num=%u", ap_num);

    if (ap_num == 0) {
#if OPT_STRONGEST_SMOOTHED_WINNER
        (void)pick_strongest_smoothed(0);
#endif
        return true;
    }

    if (ap_num > WIFI_SCAN_MAX_APS) {
        ap_num = WIFI_SCAN_MAX_APS;
    }

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, aps));

    for (int i = 0; i < (int)ap_num; i++) {
        ESP_LOGI(TAG, "[%2d] SSID %s  RSSI %d dBm  CH %d",
                 i, (char *)aps[i].ssid, aps[i].rssi, aps[i].primary);
    }

    out->ap_count = ap_num;

    for (int i = 0; i < (int)ap_num; i++) {
        out->ap_records[i].rssi    = aps[i].rssi;
        out->ap_records[i].primary = aps[i].primary;

        strncpy(out->ap_records[i].ssid,
                (char *)aps[i].ssid,
                sizeof(out->ap_records[i].ssid) - 1);
        out->ap_records[i].ssid[sizeof(out->ap_records[i].ssid) - 1] = '\0';
    }

    int best_idx;

#if OPT_STRONGEST_SMOOTHED_WINNER
    best_idx = pick_strongest_smoothed(ap_num);
    if (best_idx < 0) {
        best_idx = find_strongest_raw(ap_num);
    }
#else
    best_idx = find_strongest_raw(ap_num);
#endif

    out->best_rssi = aps[best_idx].rssi;

    strncpy(out->best_ssid,
            (char *)aps[best_idx].ssid,
            sizeof(out->best_ssid) - 1);
    out->best_ssid[sizeof(out->best_ssid) - 1] = '\0';

    ESP_LOGI(TAG, "Strongest AP SSID %s RSSI %d dBm CH %d",
             (char *)aps[best_idx].ssid,
             aps[best_idx].rssi,
             aps[best_idx].primary);

    /* Total power estimate: sum of per AP power in mW */
    float total_mw = 0.0f;

    for (int i = 0; i < (int)ap_num; i++) {
        float rssi_dbm = (float)aps[i].rssi;
        total_mw += powf(10.0f, rssi_dbm / 10.0f);
    }

    out->total_mw = total_mw;
    out->total_dbm = (total_mw > 0.0f) ? (10.0f * log10f(total_mw)) : -120.0f;

    ESP_LOGI(TAG, "Total power estimate: %.2f dBm, %.6f mW",
             out->total_dbm, out->total_mw);

    return true;
}
