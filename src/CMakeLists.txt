cmake_minimum_required (VERSION 2.6)
project (ehwe C)

include(CheckLibraryExists)
include(CheckTypeSize)
include(CheckCSourceCompiles)
include(CheckIncludeFiles)

message("CYGWIN: ${CYGWIN}")
message("CMAKE SYSTEM: ${CMAKE_SYSTEM_NAME}")

################################################################################
# Configuration options
################################################################################
# Normal project options (src defines)
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# X buildtool options (gcc)
#-------------------------------------------------------------------------------
set(SYSROOT
	${CMAKE_SYSROOT}
	CACHE STRING
	"System path (--system=)")

#-------------------------------------------------------------------------------
# System (project supported) options
#-------------------------------------------------------------------------------
# Logging: The following two work in conjunction affecting output to console
# or syslog
option(ENABLE_LOGGING
    "Enable log messages." ON)

option(ENABLE_SYSLOG
    "Enable log to syslog (Linux/OSX)" OFF)

# Init/fini
option(ENABLE_INITFINI_SHOWEXEC
    "Show init/fini execution using CTORS/DTORS mechanism" OFF)

# Self-tests
option(ENABLE_BITFIELD_TEST
    "Assure that bit-fields are organized/layedout as intended" OFF)

#-------------------------------------------------------------------------------
# Build-tool options (gcc)
#-------------------------------------------------------------------------------
set(SYSROOT
	${CMAKE_SYSROOT}
	CACHE STRING
	"System path (--system=)")

#-------------------------------------------------------------------------------
# Build-tool options
#-------------------------------------------------------------------------------

# (FIXME) Fuckit! I can't make this propagate to the build ;-(
set(CMAKE_EXTRA_C_FLAGS
	"${CMAKE_EXTRA_C_FLAGS} -DEHWE"
	CACHE STRING
	"Compiler options appended to CMAKE_C_FLAGS* (-DEHWE)")

#-------------------------------------------------------------------------------
# X build-tool options (this project's x-tool handling)
#-------------------------------------------------------------------------------
if (NOT SYSROOT STREQUAL "")
	message( "** INFO: SYSROOT was either set or defaulted from toolchain file" )
	set(CMAKE_SYSROOT "${SYSROOT}")
	set(CMAKE_EXTRA_C_FLAGS "${CMAKE_EXTRA_C_FLAGS} --sysroot=${CMAKE_SYSROOT}")
endif (NOT SYSROOT STREQUAL "")

set(CMAKE_C_FLAGS "${CMAKE_EXTRA_C_FLAGS} -no-integrated-cpp -Wno-unused-function -Wall")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_EXTRA_C_FLAGS} -g3 -ggdb3 -O0")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_EXTRA_C_FLAGS} -no-integrated-cpp -Wno-unused-function -O2 -Wall")

################################################################################
# System & host-libraries detection. Note: This logic to come before CMake
# source-bound libraries using the information
################################################################################
CHECK_INCLUDE_FILES(termio.h HAVE_POSIX_TERMIO)
if (CYGWIN)
	set(HAVE_CYGWIN "1")
endif (CYGWIN)

################################################################################
# Libraries (src binding)
################################################################################
# -Man-, help- and other generated pages
#-------------------------------------------------------------------------------
include_directories ("${PROJECT_SOURCE_DIR}/doc/include")
add_subdirectory (doc)
set (EXTRA_LIBS ${EXTRA_LIBS} doc)

if (ENABLE_LOGGING)
    include_directories ("${PROJECT_SOURCE_DIR}/lib/log/include")
    add_subdirectory (lib/log)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--undefined=__liblog_init")
	set (EXTRA_LIBS ${EXTRA_LIBS} liblog)
endif (ENABLE_LOGGING)

#-------------------------------------------------------------------------------
# -Lists-
#-------------------------------------------------------------------------------
include_directories ("${PROJECT_SOURCE_DIR}/lib/mlist/include")
add_subdirectory (lib/mlist)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--undefined=__mlist_init")
set (EXTRA_LIBS ${EXTRA_LIBS} libmlist)

#-------------------------------------------------------------------------------
# -time-
#-------------------------------------------------------------------------------
include_directories ("${PROJECT_SOURCE_DIR}/lib/mtime/include")
add_subdirectory (lib/mtime)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--undefined=__mtime_init")
set (EXTRA_LIBS ${EXTRA_LIBS} libmtime)

#-------------------------------------------------------------------------------
# -stermio (serial termio helper routines)-
#-------------------------------------------------------------------------------
if (HAVE_POSIX_TERMIO)
	include_directories ("${PROJECT_SOURCE_DIR}/lib/stermio/include")
	add_subdirectory (lib/stermio)
	set (EXTRA_LIBS ${EXTRA_LIBS} libstermio)
endif ()

#-------------------------------------------------------------------------------
# This projects source-libraries. Must be come AFTER generic libraries that
# may be conditional. 
#-------------------------------------------------------------------------------
# Devices - i.e. the real stuff behind this project
#-------------------------------------------------------------------------------
include_directories ("${PROJECT_SOURCE_DIR}/devices")
add_subdirectory (devices)
set (EXTRA_LIBS ${EXTRA_LIBS} devices)

#-------------------------------------------------------------------------------
# Interfaces - i.e. the other end of the real stuff behind this project
#-------------------------------------------------------------------------------
include_directories ("${PROJECT_SOURCE_DIR}/interfaces")
add_subdirectory (interfaces)
set (EXTRA_LIBS ${EXTRA_LIBS} interfaces)

#-------------------------------------------------------------------------------
# Embedded code
#-------------------------------------------------------------------------------
add_subdirectory (embedded)
set (EXTRA_LIBS ${EXTRA_LIBS} embedded)

################################################################################
# Project
################################################################################
# The version number.
set (EHWE_VERSION_MAJOR 0)
set (EHWE_VERSION_MINOR 0)
set (EHWE_VERSION_PATCH 1)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
  )

# add the binary tree to the search path for include files
# so that we will find Config.h
include_directories("${PROJECT_BINARY_DIR}")

set(EHWE_SOURCE
	main.c
	opts.c
	opts_help.c
)

install(PROGRAMS ${PROJECT_BINARY_DIR}/src DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

add_executable(ehwe ${EHWE_SOURCE})
target_link_libraries (ehwe ${EXTRA_LIBS})
