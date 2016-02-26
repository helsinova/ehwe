include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_SOURCE_DIR}")

################################################################################
# Configuration options for this module
################################################################################
# Normal project options (src defines)
#-------------------------------------------------------------------------------
# interface stm32 HAL library
option(INTERFACE_STM32
    "Enable interface stm32." YES)

#-------------------------------------------------------------------------------
# End of configuration options
#-------------------------------------------------------------------------------

#Top level sources
set(LIBINTERFACES_SOURCE
	interfaces.c
)

if (INTERFACE_STM32)
	# Interface-specific sources
	set(LIBINTERFACES_SOURCE
		${LIBINTERFACES_SOURCE}
		stm32.c
	)
endif (INTERFACE_STM32)

add_library(interfaces ${LIBINTERFACES_SOURCE})
