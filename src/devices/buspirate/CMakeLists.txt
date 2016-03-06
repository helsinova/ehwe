include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_SOURCE_DIR}")

option(BUSPIRATE_ENABLE_SPI
    "Enable SPI for BUSPIRATE." YES)

option(BUSPIRATE_ENABLE_I2C
    "Enable i2c for BUSPIRATE." NO)

set(LIBBUSPIRATE_SOURCE
    buspirate.c
    modechange.c
)

if(BUSPIRATE_ENABLE_SPI)
    set(LIBBUSPIRATE_SOURCE
        ${LIBBUSPIRATE_SOURCE}
        spi_raw.c
        spi_driver.c
    )
else()
    message(STATUS "Skipping Bus-pirate SPI support")
endif()

if(BUSPIRATE_ENABLE_I2C)
    set(LIBBUSPIRATE_SOURCE
        ${LIBBUSPIRATE_SOURCE}
        i2c_raw.c
        i2c_driver.c
    )
else()
    message(STATUS "Skipping Bus-pirate i2c support")
endif()

add_library(buspirate ${LIBBUSPIRATE_SOURCE})
