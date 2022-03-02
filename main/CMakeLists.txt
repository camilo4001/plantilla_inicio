# Embed the server root certificate into the final binary
idf_build_get_property(project_dir PROJECT_DIR)
idf_component_register(SRCS "main.c" "init_wifi.c" "ble_inicial.c" "ble_uart_server.c"
					"ini_http_server.c"
                    "TFT_LCD/ST7789.c" "TFT_LCD/GFX_Library.c" "TFT_LCD/tft_lcd.c"
					INCLUDE_DIRS "."
                    )
