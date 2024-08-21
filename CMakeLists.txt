idf_component_register(SRCS "src/BTD.cpp" "src/btd_vhci.cpp" "src/BTHID.cpp" "src/PS4Parser.cpp" "src/hidboot.cpp"
                    INCLUDE_DIRS "src"
                    PRIV_REQUIRES bt esp_timer nvs_flash esp_ringbuf)
