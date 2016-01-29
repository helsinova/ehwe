include (CMakeForceCompiler)
set(CMAKE_SYSTEM_NAME Generic)

set(HOME $ENV{HOME})
set(CMAKE_SYSROOT ${HOME}/usr/local/android-ndk-r10d/platforms/android-21/arch-arm)
set(CMAKE_STAGING_PREFIX /tmp/stage/Android)

# Todo: Can variable "tripple" be set from cmd-line even if it's declaration
# is optionable?
set(triple arm-linux-androideabi-)
set(CMAKE_C_COMPILER_TARGET ${triple})

#set(CMAKE_C_COMPILER ${triple}gcc)
#set(CMAKE_CXX_COMPILER ${triple}g++)

#Todo: Autodetect this by: ${triple}gcc -dM -E - < /dev/null | grep ANDROID
set(ANDROID_TARGET "yes")

CMAKE_FORCE_C_COMPILER(${triple}gcc ${triple}gcc)
CMAKE_FORCE_CXX_COMPILER(${triple}g++ ${triple}g++)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

if (NOT ANDROID_TARGET STREQUAL "")
	message( "## INFO: Build for Android target" )
	set(CMAKE_EXTRA_C_FLAGS "${CMAKE_EXTRA_C_FLAGS} -fPIE -pie")
endif (NOT ANDROID_TARGET STREQUAL "")
