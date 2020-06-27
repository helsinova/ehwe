include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${CMAKE_BINARY_DIR}/adapters")

option(BUSPIRATE_ENABLE_SPI
    "Enable SPI for BUSPIRATE." YES)

option(BUSPIRATE_ENABLE_I2C
    "Enable i2c for BUSPIRATE." YES)

set(LIBBUSPIRATE_SOURCE
    buspirate.c
    modechange.c
)

if(BUSPIRATE_ENABLE_SPI)
    set(BUSPIRATE_SPI_DFLT_SPEED
        "SPISPEED_30kHz"
        CACHE STRING
        "SPI-bus: Default clock speed (30kHz=0,125kHz=1,250kHz=2,1MHz=3,2MHz=4,2_6MHz=5,4MHz=6,8MHz=7)")

    set(BUSPIRATE_SPI_DFLT_CLK_IDLE_POLARITY
        "POL_LOW"
        CACHE STRING
        "SPI-bus: Polarity of clock when idle (POL_LOW=0,POL_HIGH=1)")

    set(BUSPIRATE_SPI_DFLT_CLK_EDGE
        "EDGE_BACK"
        CACHE STRING
        "SPI-bus: Output clock edge (EDGE_FRONT=0, EDGE_BACK=1)")

    set(BUSPIRATE_SPI_DFLT_SAMPLE
        "PULSE_MIDDLE"
        CACHE STRING
        "SPI-bus: Input sample phase (PULSE_MIDDLE=0,PULSE_END=1)")

#    set(BUSPIRATE_SPI_DFLT_CS_POL
#        "POL_LOW"
#        CACHE STRING
#        "SPI-bus: Chip-select polarity (POL_LOW=0,POL_HIGH=1)")

    set(BUSPIRATE_SPI_DFLT_OUTPUT_TYPE
        "OPEN_DRAIN"
        CACHE STRING
        "SPI-bus: Pins output type: (OPEN_DRAIN=0,PUSH_PULL=1)")

    option(BUSPIRATE_SPI_DFLT_PON
        "Power supply output-pin active" no)

    option(BUSPIRATE_SPI_DFLT_ENABLE_PULLUPS
        "Toggle on-board pull-up resistors" no)

    option(BUSPIRATE_SPI_DFLT_AUX_ON
        "Auxaliary output-pin active" no)

    option(BUSPIRATE_SPI_DFLT_CS_START_LEVEL
        "CS starts high" yes)

    set(LIBBUSPIRATE_SOURCE
        ${LIBBUSPIRATE_SOURCE}
        spi_raw.c
        spi_driver.c
    )
else()
    message(STATUS "Skipping Bus-pirate SPI support")
endif()

if(BUSPIRATE_ENABLE_I2C)
    set(BUSPIRATE_I2C_DFLT_SPEED
        "I2CSPEED_5kHz"
        CACHE STRING
        "I2C-bus: Default clock speed (5kHz=0,50kHz=1,100kHz=2,400kHz=3)")

    option(BUSPIRATE_I2C_DFLT_PON
        "Power supply output-pin active" no)

    option(BUSPIRATE_I2C_DFLT_ENABLE_PULLUPS
        "Toggle on-board pull-up resistors" no)

    option(BUSPIRATE_I2C_DFLT_AUX_ON
        "Auxaliary output-pin active" no)

    option(BUSPIRATE_I2C_DFLT_CS_START_LEVEL
        "CS starts high" yes)

    option(BUSPIRATE_I2C_DFLT_AUTOACK
        "Send ACK on each byte-read autoatically" yes)

    set(LIBBUSPIRATE_SOURCE
        ${LIBBUSPIRATE_SOURCE}
        i2c_raw.c
        i2c_driver.c
    )
else()
    message(STATUS "Skipping Bus-pirate i2c support")
endif()

include_directories("${CMAKE_CURRENT_BINARY_DIR}")
configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/buspirate_config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/buspirate_config.h"
  )

add_library(buspirate ${LIBBUSPIRATE_SOURCE})
if (HAVE_POSIX_TERMIO)
	target_link_libraries(buspirate libstermio)
endif ()

