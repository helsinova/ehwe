include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_SOURCE_DIR}")

include(CheckLibraryExists)
include(CheckTypeSize)
include(CheckCSourceCompiles)
include(CheckIncludeFiles)

set(DEF_MAX_DEVICES
    "30"
    CACHE STRING
    "Maximum number of devices")

option(DEVICE_BUSPIRATE
    "Enable device BUSPIRATE." YES)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    option(DEVICE_PARAPORT
        "Enable device PARAPORT (requires special kernel-driver)." NO)

    option(DEVICE_LXI
        "Enable device LXI - LinuX kernel-module supported bus-Interfaces." YES)
endif()


set(LIBDEVICES_SOURCE
    devices.c
)

if (DEVICE_BUSPIRATE)
    include_directories ("${PROJECT_SOURCE_DIR}/devices/buspirate/include")
    add_subdirectory (buspirate)
    set (DEVICES_LIBS ${DEVICES_LIBS} buspirate)
else()
    message(STATUS "Skipping BUSPIRATE directory")
endif()

if (DEVICE_LXI)
    include_directories ("${PROJECT_SOURCE_DIR}/devices/lxi/include")
    add_subdirectory (lxi)
    set (DEVICES_LIBS ${DEVICES_LIBS} lxi)
else()
	message(STATUS "Skipping LXI directory")
endif()

if (DEVICE_PARAPORT)
    set(LIBDEVICES_SOURCE
        ${LIBDEVICES_SOURCE}
        paraprt.c
    )
else()
    message(STATUS "Skipping PARAPORT build")
endif()

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/devices_config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/devices_config.h"
  )

add_library(devices ${LIBDEVICES_SOURCE})

target_link_libraries (devices ${DEVICES_LIBS})
