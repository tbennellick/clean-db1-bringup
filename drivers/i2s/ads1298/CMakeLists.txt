if(CONFIG_I2S_ADS1298)
    zephyr_library()
    zephyr_library_sources(ads1298_i2s.c)
endif()