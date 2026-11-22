idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_wifi esp_netif esp_event esp_timer esp_http_server
             esp_driver_uart esp_system lwip
)
