set(src_dirs
            MYUART
            LED
            OLED)

set(include_dirs
            MYUART
            LED
            OLED)
set(requires
            driver
            esp_timer
            esp_lcd esp_timer esp_lvgl_port 78__xiaozhi-fonts)

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})

component_compile_options(-ffast-math -O3 -Wno-error=format=-Wno-format)