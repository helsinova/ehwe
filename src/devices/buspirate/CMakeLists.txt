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
    set(BUSPIRATE_SPI_DFLT_SPEED_KHZ
        "30"
        CACHE STRING
        "SPI-bus: Default clock speed (30=1,125=2,250=4,1000=4)")

    set(BUSPIRATE_SPI_DFLT_CLOCK_IDLE_POLARITY
        "POL_LOW"
        CACHE STRING
        "SPI-bus: Polarity of clock when idle (POL_LOW=1,POL_HI=2)")

    set(BUSPIRATE_SPI_DFLT_CLOCK_EDGE
        "IDLE_2_ACT"
        CACHE STRING
        "SPI-bus: Output clock edge (IDLE_2_ACT=1, ACT_2_IDLE=2)")

    set(BUSPIRATE_SPI_DFLT_SAMPLE
        "MIDDLE"
        CACHE STRING
        "SPI-bus: Input sample phase (MIDDLE=1,END=2)")

    set(BUSPIRATE_SPI_DFLT_CS_POL
        "POL_LOW"
        CACHE STRING
        "SPI-bus: Chip-select polarity (POL_HI=1,POL_LOW=2)")

    set(BUSPIRATE_SPI_DFLT_OUTPUT
        "OPEN_DRAIN"
        CACHE STRING
        "SPI-bus: Pins output type: (OPEN_DRAIN=1,PUSH_PULL=2)")

    option(BUSPIRATE_SPI_DFLT_PON
        "Power supply output-pin active" yes)

    option(BUSPIRATE_SPI_DFLT_AUX_ON
        "Auxaliary output-pin active" yes)

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


configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/buspirate_config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/buspirate_config.h"
  )

add_library(buspirate ${LIBBUSPIRATE_SOURCE})
