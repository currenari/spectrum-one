/**
 * @file led_bar.h
 * @brief Public API for a 10-segment LED bar.
 *
 * Hardware control layer only.
 */

#ifndef LED_BAR_H
#define LED_BAR_H

/* C linkage when included from C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Configure all LED GPIOs as outputs and set them low.
 */
void led_bar_init(void);

/**
 * Set the bar level.
 *
 * @param level Number of segments to light (0 to 10). Values are clamped.
 */
void led_bar_set_level(int level);

#ifdef __cplusplus
}
#endif

#endif /* LED_BAR_H */
