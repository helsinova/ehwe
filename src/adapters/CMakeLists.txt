include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${CMAKE_BINARY_DIR}/adapters")

include(CheckLibraryExists)
include(CheckTypeSize)
include(CheckCSourceCompiles)
include(CheckIncludeFiles)

set(DEF_MAX_ADAPTERS
    "30"
    CACHE STRING
    "Maximum number of adapters")

# Options enabling/disabling adaptor support
# ------------------------------------------------------------------------------
option(ADAPTER_BUSPIRATE
    "Enable device BUSPIRATE." YES)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    option(ADAPTER_PARAPORT
        "Enable device PARAPORT (requires special kernel-driver)." NO)

    option(ADAPTER_LXI
        "Enable device LXI - LinuX kernel-module supported bus-Interfaces." YES)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/hif")
	option(ADAPTER_HIF "Support HIF adapter" ON)
else()
	option(ADAPTER_HIF "Support HIF adapter" OFF)
endif()
# ------------------------------------------------------------------------------

set(LIBADAPTERS_SOURCE
    adapters.c
)

if (ADAPTER_BUSPIRATE)
    include_directories ("${PROJECT_SOURCE_DIR}/adapters/buspirate/include")
    add_subdirectory (buspirate)
    set (ADAPTERS_LIBS ${ADAPTERS_LIBS} buspirate)
else()
    message(STATUS "Skipping BUSPIRATE directory")
endif()

if (ADAPTER_LXI)
    include_directories ("${PROJECT_SOURCE_DIR}/adapters/lxi/include")
    add_subdirectory (lxi)
    set (ADAPTERS_LIBS ${ADAPTERS_LIBS} lxi)
else()
	message(STATUS "Skipping LXI directory")
endif()

if (ADAPTER_PARAPORT)
    set(LIBADAPTERS_SOURCE
        ${LIBADAPTERS_SOURCE}
        paraprt.c
    )
else()
    message(STATUS "Skipping PARAPORT build")
endif()

if (ADAPTER_HIF)
    include_directories ("${CMAKE_CURRENT_SOURCE_DIR}/hif/include")
    add_subdirectory (hif)
	set (ADAPTERS_LIBS ${ADAPTERS_LIBS} hif)
else()
	message(STATUS "Skipping HIF support")
endif()

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/adapters_config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/adapters_config.h"
  )

add_library(adapters ${LIBADAPTERS_SOURCE})

target_link_libraries (adapters ${ADAPTERS_LIBS})
