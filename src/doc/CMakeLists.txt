
################################################################################
# Configuration options
################################################################################
option(BUILD_CLI_DOCUMENTATION
        "Build man page. Requires help2man."
        ${BUILD_DOCUMENTATION})

if(NOT ${BUILD_DOCUMENTATION})
    set(BUILD_CLI_DOCUMENTATION OFF)
endif()

################################################################################
# Regenerate interactive help
################################################################################
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/gen)
include_directories ("${CMAKE_CURRENT_BINARY_DIR}/gen")
find_program(PANDOC_EXECUTABLE pandoc)

if(PANDOC_EXECUTABLE)
    # Run the generation script
    message(STATUS "Will use pandoc to generate interactive help")
    add_custom_command(
        OUTPUT cmd_help.h cmd_help.man
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/bin/generate.bash ${CMAKE_CURRENT_SOURCE_DIR}/interactive-help.page
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/gen
    )
else(PANDOC_EXECUTABLE)
    message( FATAL_ERROR  "Could not find pandoc. Can't continue.")
endif(PANDOC_EXECUTABLE)

################################################################################
# Man pages
################################################################################
if(BUILD_CLI_DOCUMENTATION)
	file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/man)
    find_program(HELP2MAN_EXECUTABLE help2man)

    if(HELP2MAN_EXECUTABLE)
        message(STATUS "Will build man page for etrace.")

        if(NOT DEFINED MAN_INSTALL_DIR)
            set(MAN_INSTALL_DIR share/man)
        endif(NOT DEFINED MAN_INSTALL_DIR)

        # Build actual man page
        add_custom_command(
            OUTPUT etrace.1
            DEPENDS etrace cmd_help.man
            COMMAND ${HELP2MAN_EXECUTABLE} --no-info --no-discard-stderr
                                           --opt-include=${CMAKE_CURRENT_SOURCE_DIR}/etrace.h2m
                                           --opt-include=${CMAKE_CURRENT_BINARY_DIR}/gen/cmd_help.man
                                           --opt-include=${CMAKE_CURRENT_SOURCE_DIR}/common.h2m
                                           --output=etrace.1 ${PROJECT_BINARY_DIR}/etrace
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/man
        )

        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/man/etrace.1 DESTINATION ${MAN_INSTALL_DIR}/man1)

        add_custom_target(etrace-manpage ALL DEPENDS etrace.1)

    else(HELP2MAN_EXECUTABLE)
        message(FATAL_ERROR "Could not find help2man. etrace man page cannot be built.")
    endif(HELP2MAN_EXECUTABLE)
else(BUILD_CLI_DOCUMENTATION)
    message(STATUS "Not building man page for etrace.  (BUILD_CLI_DOCUMENTATION is OFF)")
endif(BUILD_CLI_DOCUMENTATION)

set(DOC_SOURCE
	cmd_help.h
	doc.c
)

add_library(doc ${DOC_SOURCE})
