include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_SOURCE_DIR}")

################################################################################
# Configuration options for this module
################################################################################
# Normal project options (src defines)
#-------------------------------------------------------------------------------
set(DEF_MAX_DEVICES
	"30"
	CACHE STRING
	"Maximum number of devices")

# Device bus-pirate
option(DEVICE_BUSPIRATE
    "Enable device BUSPIRATE." YES)

# Device paraport
option(DEVICE_PARAPORT
    "Enable device PARAPORT (requires special kernel-driver)." NO)

#-------------------------------------------------------------------------------
# End of configuration options
#-------------------------------------------------------------------------------

#Top level sources
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--undefined=__devices_init")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--undefined=__devices_init")
set(LIBDEVICES_SOURCE
	devices.c
	devices_initfini.c
)

if (DEVICE_BUSPIRATE)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--undefined=__buspirate_init")
	set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--undefined=__buspirate_init")
	# Device-specific sources
	set(LIBDEVICES_SOURCE
		${LIBDEVICES_SOURCE}
		buspirate_initfini.c
		buspirate.c
	)
endif (DEVICE_BUSPIRATE)

if (DEVICE_PARAPORT)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--undefined=__paraport_init")
	set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--undefined=__paraport_init")
	# Device-specific sources
	set(LIBDEVICES_SOURCE
		${LIBDEVICES_SOURCE}
		paraport_initfini.c
		paraprt.c
	)
endif (DEVICE_PARAPORT)

add_library(devices ${LIBDEVICES_SOURCE})
