include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_SOURCE_DIR}")

################################################################################
# Configuration options for this module
################################################################################
# Normal project options (src defines)
#-------------------------------------------------------------------------------
# interface stm32 HAL library
option(ENABLE_API_STM32
    "Enable interface stm32." YES)

# Enebling this will make some old workbenhes not build, therefore disabled
# by default
option(ENABLE_API_HIGH_LVL
	"Enable high-level API similar to Nordic component's HAL" NO)

#-------------------------------------------------------------------------------
# End of configuration options
#-------------------------------------------------------------------------------
#Top level common sources
set(LIBINTERFACES_SOURCE
	interfaces.c
)

#-------------------------------------------------------------------------------
# Interface-specific sources
#-------------------------------------------------------------------------------
if (ENABLE_API_HIGH_LVL)
	set (LIBINTERFACES_SOURCE
		${LIBINTERFACES_SOURCE}
		ehwe.c
	)
endif (ENABLE_API_HIGH_LVL)

if (ENABLE_API_STM32)
	include_directories ("${PROJECT_SOURCE_DIR}/interfaces/include")
	set(LIBINTERFACES_SOURCE
		${LIBINTERFACES_SOURCE}
		stm32.c
	)
endif (ENABLE_API_STM32)

add_library(interfaces ${LIBINTERFACES_SOURCE})
