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
set(LIBDEVICES_SOURCE
	devices.c
)

if (DEVICE_BUSPIRATE)
	# Device-specific sources
	set(LIBDEVICES_SOURCE
		${LIBDEVICES_SOURCE}
		buspirate.c
	)
endif (DEVICE_BUSPIRATE)

if (DEVICE_PARAPORT)
	# Device-specific sources
	set(LIBDEVICES_SOURCE
		${LIBDEVICES_SOURCE}
		paraprt.c
	)
endif (DEVICE_PARAPORT)

add_library(devices ${LIBDEVICES_SOURCE})
