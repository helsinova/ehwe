include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${CMAKE_BINARY_DIR}/adapters")

include_directories(/usr/include)
include_directories(/usr/local/include)

include(CheckLibraryExists)
include(CheckTypeSize)
include(CheckIncludeFiles)

CHECK_INCLUDE_FILES(linux/i2c-dev.h HAVE_LINUX_I2C-DEV_H)

if(HAVE_LINUX_I2C-DEV_H)
    option(LXI_ENABLE_SPI
        "Enable SPI for LXI (Linux only)" YES)

    option(LXI_ENABLE_I2C
        "Enable i2c for LXI (Linux only)" YES)
endif()

set(LIBLXI_SOURCE
    lxi.c
)

if(LXI_ENABLE_SPI)
    # HW-config would usually go here. Not much to do for us as we don't
    # have that detailed control.

    set(LIBLXI_SOURCE
        ${LIBLXI_SOURCE}
        spi_driver.c
    )
else()
    message(STATUS "Skipping LXI SPI support")
endif()

if(LXI_ENABLE_I2C)
    # HW-config would usually go here. Not much to do for us as we don't
    # have that detailed control.

    set(LIBLXI_SOURCE
        ${LIBLXI_SOURCE}
        i2c_driver.c
    )
else()
    message(STATUS "Skipping LXI i2c support")
endif()


include_directories("${CMAKE_CURRENT_BINARY_DIR}")
configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/lxi_config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/lxi_config.h"
  )

add_library(lxi ${LIBLXI_SOURCE})

