idf_component_register(SRCS "PMW-Wifi.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_wifi EspOSInterface
                    PRIV_REQUIRES esp_event nvs_flash)
