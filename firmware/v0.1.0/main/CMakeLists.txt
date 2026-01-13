# Spectrum One firmware component
idf_component_register(
    SRCS
        "spectrum_one.c"
        "wifi_scan.c"
        "led_bar.c"
        "lcd1602.c"
        "wifi_one_math.c"
    INCLUDE_DIRS
        "."
)
