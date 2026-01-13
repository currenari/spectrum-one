#ifndef SPECTRUM_ONE_CONFIG_H
#define SPECTRUM_ONE_CONFIG_H

/*
 * Spectrum One hardware configuration.
 * Reference build: v0.1.0
 */

#include "driver/gpio.h"
#include "driver/i2c.h"

/* ---------------- I2C LCD ---------------- */

/* I2C configuration for LCD1602 backpack */
#define SO_I2C_PORT        I2C_NUM_0
#define SO_I2C_SDA_GPIO    16
#define SO_I2C_SCL_GPIO    17
#define SO_I2C_FREQ_HZ     100000

/* PCF8574 I2C address */
#define SO_LCD_ADDR        0x27

/* ---------------- User input ---------------- */

/* Active-low push button */
#define SO_BUTTON_GPIO     GPIO_NUM_13

/* ---------------- LED bar graph ---------------- */

/* 10-segment LED bar graph GPIO mapping */
#define SO_LED1_GPIO       GPIO_NUM_32
#define SO_LED2_GPIO       GPIO_NUM_33
#define SO_LED3_GPIO       GPIO_NUM_25
#define SO_LED4_GPIO       GPIO_NUM_26
#define SO_LED5_GPIO       GPIO_NUM_27
#define SO_LED6_GPIO       GPIO_NUM_18
#define SO_LED7_GPIO       GPIO_NUM_19
#define SO_LED8_GPIO       GPIO_NUM_21
#define SO_LED9_GPIO       GPIO_NUM_22
#define SO_LED10_GPIO      GPIO_NUM_23

#endif /* SPECTRUM_ONE_CONFIG_H */

