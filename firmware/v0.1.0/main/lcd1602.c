/**
 * @file lcd1602.c
 * @brief HD44780-compatible 16x2 LCD driver using a PCF8574 I2C backpack.
 *
 * Reference implementation for Spectrum One v0.1.0.
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "esp_rom_sys.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lcd1602.h"
#include "config.h"

static const char *TAG = "LCD1602";

// -----------------------------------------------------------------------------
// I2C configuration and LCD constants
// -----------------------------------------------------------------------------

#define I2C_PORT       SO_I2C_PORT
#define I2C_SDA_GPIO   SO_I2C_SDA_GPIO
#define I2C_SCL_GPIO   SO_I2C_SCL_GPIO
#define I2C_FREQ_HZ    SO_I2C_FREQ_HZ

#define LCD_ADDR       SO_LCD_ADDR

/* PCF8574 output bit mapping */
#define LCD_ENABLE     0x04
#define LCD_RW         0x02
#define LCD_RS         0x01

/* Backlight control bit */
#define LCD_BL_ON      0x08
#define LCD_BL_OFF     0x00

static bool lcd_ok = false;

/* Current backlight state */
static uint8_t g_bl = LCD_BL_ON;

static inline uint8_t with_bl(uint8_t v)
{
    return (uint8_t)((v & (uint8_t)~0x08) | g_bl);
}

static void lcd_backlight(int on)
{
    g_bl = on ? LCD_BL_ON : LCD_BL_OFF;

    /* Push backlight state without toggling control lines */
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == NULL) return;

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (LCD_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, with_bl(0x00), true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
}

// -----------------------------------------------------------------------------
// Low level I2C helpers
// -----------------------------------------------------------------------------

static esp_err_t lcd_i2c_write(uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == NULL) return ESP_FAIL;

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (LCD_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, with_bl(data), true);
    i2c_master_stop(cmd);

    esp_err_t res = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return res;
}

static void lcd_pulse_enable(uint8_t data)
{
    lcd_i2c_write((uint8_t)(data | LCD_ENABLE));
    esp_rom_delay_us(1);

    lcd_i2c_write((uint8_t)(data & (uint8_t)~LCD_ENABLE));
    esp_rom_delay_us(50);
}

/* Nibble must be aligned to bits D7..D4 */
static void lcd_send_nibble(uint8_t nibble_aligned, uint8_t mode)
{
    uint8_t data = (uint8_t)((nibble_aligned & 0xF0) | mode);
    lcd_pulse_enable(data);
}

static void lcd_send_byte(uint8_t value, uint8_t mode)
{
    lcd_send_nibble((uint8_t)(value & 0xF0), mode);
    lcd_send_nibble((uint8_t)((value << 4) & 0xF0), mode);
}

static void lcd_cmd(uint8_t cmd)
{
    lcd_send_byte(cmd, 0x00);
    vTaskDelay(pdMS_TO_TICKS(2));
}

static void lcd_data(uint8_t data)
{
    lcd_send_byte(data, LCD_RS);
}

/* Refactor candidate: overlaps with lcd_write_string() */
static void lcd_print(const char *str)
{
    while (*str) {
        lcd_data((uint8_t)*str++);
    }
}

// -----------------------------------------------------------------------------
// High level helpers
// -----------------------------------------------------------------------------

static void lcd_clear(void)
{
    lcd_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(2));
}

static void lcd_home(void)
{
    lcd_cmd(0x02);
    vTaskDelay(pdMS_TO_TICKS(2));
}

static void lcd_set_cursor(uint8_t col, uint8_t row)
{
    uint8_t addr = (row == 0) ? (uint8_t)(0x00 + col) : (uint8_t)(0x40 + col);
    lcd_cmd((uint8_t)(0x80 | addr));
}

static void lcd_write_string(const char *str)
{
    while (*str) {
        lcd_data((uint8_t)*str++);
    }
}

static void lcd_clear_line(uint8_t row)
{
    lcd_set_cursor(0, row);
    for (int i = 0; i < 16; i++) {
        lcd_data(' ');
    }
}

static void lcd_write_line_clipped(uint8_t row, const char *s)
{
    char buf[17];
    for (int i = 0; i < 16; i++) buf[i] = ' ';
    buf[16] = '\0';

    if (s != NULL) {
        size_t n = strlen(s);
        if (n > 16) n = 16;
        for (size_t i = 0; i < n; i++) {
            buf[i] = s[i];
        }
    }

    lcd_clear_line(row);
    lcd_set_cursor(0, row);
    lcd_write_string(buf);
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

void lcd_init(void)
{
    esp_err_t err;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
        .clk_flags = 0
    };

    err = i2c_param_config(I2C_PORT, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_param_config failed: %s", esp_err_to_name(err));
        return;
    }

    err = i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_driver_install failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "I2C ready SDA=%d SCL=%d", I2C_SDA_GPIO, I2C_SCL_GPIO);

    vTaskDelay(pdMS_TO_TICKS(50));

    lcd_backlight(1);

    if (lcd_i2c_write(0x00) != ESP_OK) {
        ESP_LOGE(TAG, "LCD not responding at 0x%02X", LCD_ADDR);
        return;
    }

    ESP_LOGI(TAG, "LCD responds at 0x%02X", LCD_ADDR);

    lcd_send_nibble(0x30, 0x00);
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_send_nibble(0x30, 0x00);
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_send_nibble(0x30, 0x00);
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_send_nibble(0x20, 0x00);

    lcd_cmd(0x28);
    lcd_cmd(0x0C);
    lcd_cmd(0x06);

    lcd_clear();
    lcd_home();

    lcd_ok = true;
    ESP_LOGI(TAG, "LCD initialised");
}

void lcd_show_status(const char *line1, const char *line2)
{
    if (!lcd_ok) return;

    lcd_write_line_clipped(0, line1);
    lcd_write_line_clipped(1, line2);
}

void lcd_show_scan_result(int best_rssi, uint16_t ap_num, const char *ssid)
{
    if (!lcd_ok) return;

    char line1[32];
    snprintf(line1, sizeof(line1), "RSSI %ddBm AP%u", best_rssi, ap_num);

    lcd_clear_line(0);
    lcd_set_cursor(0, 0);
    lcd_write_string(line1);

    char line2[32];

    if (ssid == NULL || ssid[0] == '\0') {
        strncpy(line2, "HIDDEN SSID", 16);
        line2[15] = '\0';
    } else {
        char name_buf[11];
        strncpy(name_buf, ssid, 10);
        name_buf[10] = '\0';
        snprintf(line2, sizeof(line2), "SSID %s", name_buf);
    }

    lcd_clear_line(1);
    lcd_set_cursor(0, 1);
    lcd_write_string(line2);
}

void lcd_show_total_power(float total_dbm, float total_mw)
{
    if (!lcd_ok) return;

    char line1[32];
    char line2[32];

    snprintf(line1, sizeof(line1), "SUM %6.1fdBm", total_dbm);
    snprintf(line2, sizeof(line2), "PWR %1.6fmW", total_mw);

    lcd_clear_line(0);
    lcd_set_cursor(0, 0);
    lcd_write_string(line1);

    lcd_clear_line(1);
    lcd_set_cursor(0, 1);
    lcd_write_string(line2);
}

void lcd_show_field_level(float field_nw)
{
    if (!lcd_ok) return;

    float density_nw_m2 = (field_nw > 0.0f) ? field_nw : 0.0f;

    char line1[32];
    char line2[32];

    snprintf(line1, sizeof(line1), "Field Level");

    if (density_nw_m2 < 9999.5f) {
        snprintf(line2, sizeof(line2), "%4.0f nW/m2", density_nw_m2);
    } else if (density_nw_m2 < 999999.5f) {
        snprintf(line2, sizeof(line2), "%.1fk nW/m2", density_nw_m2 / 1000.0f);
    } else {
        snprintf(line2, sizeof(line2), "%.1fM nW/m2", density_nw_m2 / 1e6f);
    }

    lcd_clear_line(0);
    lcd_set_cursor(0, 0);
    lcd_print(line1);

    lcd_clear_line(1);
    lcd_set_cursor(0, 1);
    lcd_print(line2);
}

void lcd_show_welcome(void)
{
    if (!lcd_ok) return;

    const int width = 16;
    char line[width + 1];

    const char *brand  = "Currenari Lab";
    const char *phase1 = "Designed in";
    const char *phase2 = "England";
    const char *phase3 = "Spectrum One";
    const char *phase4 = "v0.1.0";

    int brand_len  = (int)strlen(brand);
    int phase1_len = (int)strlen(phase1);
    int phase2_len = (int)strlen(phase2);
    int phase3_len = (int)strlen(phase3);
    int phase4_len = (int)strlen(phase4);

    int brand_pad  = (width - brand_len)  / 2;
    int phase1_pad = (width - phase1_len) / 2;
    int phase2_pad = (width - phase2_len) / 2;
    int phase3_pad = (width - phase3_len) / 2;
    int phase4_pad = (width - phase4_len) / 2;

    for (int i = 0; i < width; i++) line[i] = ' ';
    for (int i = 0; i < brand_len; i++) {
        int pos = brand_pad + i;
        if (pos >= 0 && pos < width) line[pos] = brand[i];
    }
    line[width] = '\0';

    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_write_string(line);

    #define SHOW_BOTTOM(text_len, text_pad, text_str)      \
        do {                                               \
            for (int i = 0; i < width; i++) line[i] = ' '; \
            for (int i = 0; i < (text_len); i++) {         \
                int pos = (text_pad) + i;                  \
                if (pos >= 0 && pos < width) {             \
                    line[pos] = (text_str)[i];             \
                }                                          \
            }                                              \
            line[width] = '\0';                            \
            lcd_clear_line(1);                             \
            lcd_set_cursor(0, 1);                          \
            lcd_write_string(line);                        \
            vTaskDelay(pdMS_TO_TICKS(2000));               \
        } while (0)

    SHOW_BOTTOM(phase1_len, phase1_pad, phase1);
    SHOW_BOTTOM(phase2_len, phase2_pad, phase2);
    SHOW_BOTTOM(phase3_len, phase3_pad, phase3);
    SHOW_BOTTOM(phase4_len, phase4_pad, phase4);

    #undef SHOW_BOTTOM
}
