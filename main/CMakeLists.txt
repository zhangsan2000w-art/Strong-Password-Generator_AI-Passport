idf_component_register(
    SRCS "main.c"
         "password_app.c"
         "password_platform.c"
         "password_sound.c"
         "settings_screen.c"
         "settings_store.c"
         "passport_font_zh_16.c"
         "ui_pixel.c"
         "ui_pixel_math.c"
    INCLUDE_DIRS "."
    REQUIRES bsp bootloader_support esp_hw_support mbedtls moonbit_password nvs_flash
)
