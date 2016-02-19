include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_SOURCE_DIR}")

#Top level sources
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--undefined=__devices_init")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--undefined=__devices_init")
set(LIBDEVICES_SOURCE
	devices.c
	devices_initfini.c
)

if (ENABLE_BUSPIRATE)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--undefined=__buspirate_init")
	set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--undefined=__buspirate_init")
	# Device-specific sources
	set(LIBDEVICES_SOURCE
		${LIBDEVICES_SOURCE}
		buspirate_initfini.c
		buspirate.c
	)
endif (ENABLE_BUSPIRATE)
if (ENABLE_PARAPORT)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--undefined=__paraport_init")
	set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--undefined=__paraport_init")
	# Device-specific sources
	set(LIBDEVICES_SOURCE
		${LIBDEVICES_SOURCE}
		paraport_initfini.c
		paraprt.c
	)
endif (ENABLE_PARAPORT)

add_library(devices ${LIBDEVICES_SOURCE})
