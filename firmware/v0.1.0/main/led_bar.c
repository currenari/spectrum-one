/**
 * @file led_bar.c
 * @brief Driver for a 10-segment LED bar using individual GPIO pins.
 *
 * GPIO control only. No WiFi or signal processing logic.
 */

#include "led_bar.h"
#include "config.h"

#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "LED_BAR";

/*
 * GPIO mapping for the 10-segment LED bar.
 * Order is segment 1 (index 0) through segment 10 (index 9).
 */
static const gpio_num_t led_pins[10] = {
    SO_LED1_GPIO,
    SO_LED2_GPIO,
    SO_LED3_GPIO,
    SO_LED4_GPIO,
    SO_LED5_GPIO,
    SO_LED6_GPIO,
    SO_LED7_GPIO,
    SO_LED8_GPIO,
    SO_LED9_GPIO,
    SO_LED10_GPIO
};

void led_bar_init(void)
{
    uint64_t mask = 0;

    for (int i = 0; i < 10; i++) {
        mask |= (1ULL << led_pins[i]);
    }

    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = mask
    };

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %d", err);
    }

    /* Initialise all segments off */
    for (int i = 0; i < 10; i++) {
        gpio_set_level(led_pins[i], 0);
    }
}

void led_bar_set_level(int level)
{
    if (level < 0)  level = 0;
    if (level > 10) level = 10;

    for (int i = 0; i < 10; i++) {
        gpio_set_level(led_pins[i], (i < level) ? 1 : 0);
    }
}
