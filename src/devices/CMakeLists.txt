include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_SOURCE_DIR}")

#Top level sources
set(LIBDEVICES_SOURCE
	devices.c
)

if (ENABLE_BUSPIRATE)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--undefined=__bp_init")
	set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--undefined=__bp_init")
	# Device-specific sources
	set(LIBDEVICES_SOURCE
		${LIBDEVICES_SOURCE}
		buspirate_initfini.c
		buspirate.c
	)
endif (ENABLE_BUSPIRATE)

add_library(devices ${LIBDEVICES_SOURCE})
