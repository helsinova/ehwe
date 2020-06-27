include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_BINARY_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${CMAKE_BINARY_DIR}/adapters")

################################################################################
# Configuration options for this module
################################################################################
# Normal project options (src defines)
#-------------------------------------------------------------------------------
# api stm32 HAL library
option(ENABLE_API_STM32
    "Enable api stm32." YES)

# NOTE: Enebling this will make some old workbenhes not build
option(ENABLE_API_HIGH_LVL
	"Enable high-level API similar to Nordic component's HAL" YES)

#-------------------------------------------------------------------------------
# End of configuration options
#-------------------------------------------------------------------------------
#Top level common sources
set(LIBINTERFACES_SOURCE
	apis.c
)

#-------------------------------------------------------------------------------
# Interface-specific sources
#-------------------------------------------------------------------------------
if (ENABLE_API_HIGH_LVL)
	set (LIBINTERFACES_SOURCE
		${LIBINTERFACES_SOURCE}
		ehwe.c
		ehwe_i2c_device.c
	)
endif (ENABLE_API_HIGH_LVL)

if (ENABLE_API_STM32)
	include_directories ("${PROJECT_SOURCE_DIR}/apis/include")
	set(LIBINTERFACES_SOURCE
		${LIBINTERFACES_SOURCE}
		stm32.c
	)
endif (ENABLE_API_STM32)

add_library(apis ${LIBINTERFACES_SOURCE})
