/**
 * @file spectrum_one.c
 *
 * Main application code for Spectrum One.
 *
 * Handles UI state, button input, display updates, and coordination
 * between WiFi scan results, the LCD, and the LED bar.
 *
 * UI modes:
 *   0 : Strongest access point (browse overlay when active)
 *   1 : Total power (SUM)
 *   2 : Field level
 *
 * Additional state flags control browsing and follow behaviour.
 *
 * Compile-time options below allow different timeout and interaction
 * behaviours to be tested.
 */


/*
 * Button response timing
 *
 * Button input is polled and debounced, then handled in the main loop.
 * This keeps UI state changes simple but adds noticeable latency.
 *
 * Delay comes from:
 * - Poll interval and debounce
 * - Event handling tied to the main loop timing
 *
 * Worst case response can be close to the main loop delay.
 *
 * If faster response is needed in a future revision:
 * - Lower debounce or poll interval
 * - Shorten the main loop delay
 * - Handle button events directly in the button task
 */



#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "driver/gpio.h"

#include "led_bar.h"
#include "wifi_scan.h"
#include "lcd1602.h"
#include "wifi_one_math.h"
#include "config.h"

static const char *TAG = "SPECTRUM_ONE";

// -----------------------------------------------------------------------------
// Button and timing configuration
// -----------------------------------------------------------------------------

#define BTN_GPIO               SO_BUTTON_GPIO

#define BUTTON_POLL_MS         10
#define DEBOUNCE_TICKS         3
#define LONG_PRESS_MS          600

// -----------------------------------------------------------------------------
// UI behaviour options (compile-time)
// -----------------------------------------------------------------------------

/*
 * Main idle timeout.
 * When enabled, the UI returns to the Field screen after inactivity.
 */
#define UI_IDLE_TIMEOUT_ENABLED     0
#define UI_IDLE_TIMEOUT_MS          8000

/*
 * Browse timeout.
 * When enabled, browse mode exits automatically after a delay.
 */
#define UI_BROWSE_TIMEOUT_ENABLED   1
#define UI_BROWSE_TIMEOUT_MS        5000

/*
 * SUM screen timeout.
 * Independent timeout for the SUM screen only.
 */
#define UI_SUM_TIMEOUT_ENABLED      0
#define UI_SUM_TIMEOUT_MS           15000

/*
 * Short-click behaviour on SUM screen.
 * When enabled, a short click exits SUM directly to Field.
 */
#define UI_SUM_SHORTCLICK_EXITS     0

/* Internal compatibility mappings */
#define AP_BROWSE_TIMEOUT_MS        UI_BROWSE_TIMEOUT_MS
#define MAIN_IDLE_TIMEOUT_MS        UI_IDLE_TIMEOUT_MS

// -----------------------------------------------------------------------------
// UI event queue
// -----------------------------------------------------------------------------

typedef enum {
    UI_EVT_SHORT_CLICK = 0,
    UI_EVT_LONG_PRESS  = 1
} ui_evt_type_t;

typedef struct {
    ui_evt_type_t type;
    TickType_t    tick;
} ui_evt_t;

static QueueHandle_t ui_evt_q = NULL;

// -----------------------------------------------------------------------------
// UI state
// -----------------------------------------------------------------------------

/* 0 = strongest, 1 = SUM, 2 = Field */
static int ui_mode = 2;

static int        current_ap_index    = 0;
static bool       ap_browse_active    = false;
static TickType_t ap_browse_last_tick = 0;

static TickType_t last_activity_tick  = 0;
static TickType_t sum_enter_tick      = 0;

static wifi_scan_result_t last_scan_result;
static bool               last_scan_valid = false;

/* Follow mode state */
static bool follow_mode     = false;
static int  follow_ap_index = -1;
static char follow_ssid[33];

/* Button debounce and press tracking */
static int        btn_stable_state = 1;
static int        btn_debounce_cnt = 0;
static bool       btn_down         = false;
static bool       btn_long_handled = false;
static TickType_t btn_down_tick    = 0;

// -----------------------------------------------------------------------------
// Button handling
// -----------------------------------------------------------------------------

static void button_init(void)
{
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = 1ULL << BTN_GPIO
    };

    gpio_config(&io_conf);

    btn_stable_state = 1;
    btn_debounce_cnt = 0;
    btn_down         = false;
    btn_long_handled = false;
}

static void ui_evt_send(ui_evt_type_t type, TickType_t now)
{
    if (ui_evt_q == NULL) return;

    ui_evt_t e = { .type = type, .tick = now };

/* Queue overflow drops events under heavy input */
    (void)xQueueSend(ui_evt_q, &e, 0);
}

// -----------------------------------------------------------------------------
// Display update logic
// -----------------------------------------------------------------------------

static void refresh_display(void)
{
    if (!last_scan_valid) {
        lcd_show_status("Scanning", "Please wait");
        led_bar_set_level(0);
        return;
    }

    /* Follow mode overrides all other UI states */
    if (follow_mode) {
        if (last_scan_result.ap_count == 0 ||
            follow_ap_index < 0 ||
            follow_ap_index >= last_scan_result.ap_count) {

            lcd_show_status("Locked AP", "Lost");
            led_bar_set_level(0);
            return;
        }

        const wifi_ap_info_t *ap = &last_scan_result.ap_records[follow_ap_index];

        lcd_show_scan_result(ap->rssi, follow_ap_index + 1, ap->ssid);
        led_bar_set_level(wifi_one_led_from_ap_rssi(ap->rssi));
        return;
    }

    if (last_scan_result.ap_count == 0) {
        lcd_show_status("No APs", "Scan again");
        led_bar_set_level(0);
        return;
    }

    int   led_level = 0;
    float field_nw  = wifi_one_field_from_mw(last_scan_result.total_mw);

    if (ui_mode == 0) {
        if (ap_browse_active) {
            if (current_ap_index >= last_scan_result.ap_count) {
                current_ap_index = 0;
            }

            const wifi_ap_info_t *ap =
                &last_scan_result.ap_records[current_ap_index];

            lcd_show_scan_result(ap->rssi,
                                 current_ap_index + 1,
                                 ap->ssid);

            led_level = wifi_one_led_from_ap_rssi(ap->rssi);

        } else {
            lcd_show_scan_result(last_scan_result.best_rssi,
                                 last_scan_result.ap_count,
                                 last_scan_result.best_ssid);

            led_level = wifi_one_led_from_field(field_nw);
        }

    } else if (ui_mode == 1) {
        lcd_show_total_power(last_scan_result.total_dbm,
                             last_scan_result.total_mw);
        led_level = wifi_one_led_from_field(field_nw);

    } else {
        lcd_show_field_level(field_nw);
        led_level = wifi_one_led_from_field(field_nw);
    }

    led_bar_set_level(led_level);
}

// -----------------------------------------------------------------------------
// UI event handlers
// -----------------------------------------------------------------------------

static void handle_short_click(TickType_t now)
{
    last_activity_tick = now;

    if (ap_browse_active &&
        last_scan_valid &&
        last_scan_result.ap_count > 0) {

        current_ap_index++;
        if (current_ap_index >= last_scan_result.ap_count) {
            current_ap_index = 0;
        }

        ap_browse_last_tick = now;
        refresh_display();
        return;
    }

#if UI_SUM_SHORTCLICK_EXITS
    if (ui_mode == 1) {
        ui_mode = 2;
        refresh_display();
        return;
    }
#endif

    if (ui_mode == 2) {
        ui_mode = 0;
    } else if (ui_mode == 0) {
        ui_mode = 1;
        sum_enter_tick = now;
    } else {
        ui_mode = 2;
    }

    if (ui_mode != 0) {
        ap_browse_active = false;
    }

    refresh_display();
}

static void handle_long_press(TickType_t now)
{
    last_activity_tick = now;

    if (follow_mode) {
        follow_mode      = false;
        follow_ap_index  = -1;
        follow_ssid[0]   = '\0';
        ap_browse_active = false;
        ui_mode          = 2;

        refresh_display();
        return;
    }

    if (ap_browse_active &&
        last_scan_valid &&
        last_scan_result.ap_count > 0) {

        const wifi_ap_info_t *ap =
            &last_scan_result.ap_records[current_ap_index];

        follow_mode     = true;
        follow_ap_index = current_ap_index;

        if (ap->ssid[0] != '\0') {
            strncpy(follow_ssid,
                    ap->ssid,
                    sizeof(follow_ssid) - 1);
            follow_ssid[sizeof(follow_ssid) - 1] = '\0';
        } else {
            follow_ssid[0] = '\0';
        }

        ap_browse_active = false;
        refresh_display();
        return;
    }

    if (last_scan_valid && last_scan_result.ap_count > 0) {
        ui_mode          = 0;
        ap_browse_active = true;

        if (current_ap_index >= last_scan_result.ap_count) {
            current_ap_index = 0;
        }

        ap_browse_last_tick = now;
        refresh_display();
    }
}

// -----------------------------------------------------------------------------
// Button polling task
// -----------------------------------------------------------------------------

static void button_task(void *arg)
{
    (void)arg;

    while (1) {
        TickType_t now = xTaskGetTickCount();

        int raw = gpio_get_level(BTN_GPIO);

        if (raw == btn_stable_state) {
            btn_debounce_cnt = 0;
        } else if (++btn_debounce_cnt >= DEBOUNCE_TICKS) {
            btn_stable_state = raw;
            btn_debounce_cnt = 0;
        }

        int state = btn_stable_state;

        if (state == 0 && !btn_down) {
            btn_down         = true;
            btn_long_handled = false;
            btn_down_tick    = now;
        }

        if (state == 0 && btn_down && !btn_long_handled) {
            if ((now - btn_down_tick) > pdMS_TO_TICKS(LONG_PRESS_MS)) {
                btn_long_handled = true;
                ui_evt_send(UI_EVT_LONG_PRESS, now);
            }
        }

        if (state == 1 && btn_down) {
            btn_down = false;
            if (!btn_long_handled) {
                ui_evt_send(UI_EVT_SHORT_CLICK, now);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_MS));
    }
}

// -----------------------------------------------------------------------------
// Application entry point
// -----------------------------------------------------------------------------

void app_main(void)
{
    led_bar_init();
    lcd_init();
    vTaskDelay(pdMS_TO_TICKS(1000));

    wifi_scan_init();
    button_init();

    ui_evt_q = xQueueCreate(8, sizeof(ui_evt_t));

    lcd_show_welcome();

    ui_mode            = 2;
    last_activity_tick = xTaskGetTickCount();
    sum_enter_tick     = last_activity_tick;
    follow_ssid[0]     = '\0';

    xTaskCreate(button_task, "button_task", 2048, NULL, 5, NULL);

    while (1) {
        ui_evt_t e;
        while (ui_evt_q &&
               xQueueReceive(ui_evt_q, &e, 0) == pdTRUE) {

            if (e.type == UI_EVT_SHORT_CLICK) {
                handle_short_click(e.tick);
            } else {
                handle_long_press(e.tick);
            }
        }

#if UI_BROWSE_TIMEOUT_ENABLED
        TickType_t now = xTaskGetTickCount();

        if (ap_browse_active &&
            (now - ap_browse_last_tick) >
            pdMS_TO_TICKS(AP_BROWSE_TIMEOUT_MS)) {

            ap_browse_active = false;
            ui_mode          = 2;
            refresh_display();
        }
#endif

#if UI_SUM_TIMEOUT_ENABLED
        if (!follow_mode &&
            !ap_browse_active &&
            ui_mode == 1 &&
            (xTaskGetTickCount() - sum_enter_tick) >
            pdMS_TO_TICKS(UI_SUM_TIMEOUT_MS)) {

            ui_mode = 2;
            refresh_display();
        }
#endif

#if UI_IDLE_TIMEOUT_ENABLED
        if (!follow_mode &&
            !ap_browse_active &&
            ui_mode != 2 &&
            (xTaskGetTickCount() - last_activity_tick) >
            pdMS_TO_TICKS(MAIN_IDLE_TIMEOUT_MS)) {

            ui_mode = 2;
            refresh_display();
        }
#endif

        wifi_scan_result_t result;

        if (follow_mode) {
            if (wifi_scan_run(&result)) {
                last_scan_result = result;
                last_scan_valid  = true;

                follow_ap_index = -1;

                for (int i = 0; i < result.ap_count; i++) {
                    const wifi_ap_info_t *ap =
                        &result.ap_records[i];

                    if (ap->ssid[0] &&
                        strncmp(ap->ssid,
                                follow_ssid,
                                sizeof(follow_ssid)) == 0) {

                        follow_ap_index = i;
                        break;
                    }
                }

            } else {
                last_scan_valid = false;
                follow_mode     = false;
                follow_ap_index = -1;
                follow_ssid[0]  = '\0';
            }

        } else if (!ap_browse_active) {
            if (wifi_scan_run(&result)) {
                last_scan_result = result;
                last_scan_valid  = true;
            } else {
                last_scan_valid   = false;
                ap_browse_active  = false;
                follow_mode       = false;
                follow_ap_index   = -1;
                follow_ssid[0]    = '\0';
            }
        }

        refresh_display();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
