include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_SOURCE_DIR}")

set(DEF_MAX_DEVICES
    "30"
    CACHE STRING
    "Maximum number of devices")

option(DEVICE_BUSPIRATE
    "Enable device BUSPIRATE." YES)

option(DEVICE_PARAPORT
    "Enable device PARAPORT (requires special kernel-driver)." NO)


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

if (DEVICE_PARAPORT)
    set(LIBDEVICES_SOURCE
        ${LIBDEVICES_SOURCE}
        paraprt.c
    )
else()
    message(STATUS "Skipping PARAPORT build")
endif()

add_library(devices ${LIBDEVICES_SOURCE})

target_link_libraries (devices ${DEVICES_LIBS})
