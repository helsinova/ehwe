#! /bin/bash

# Convenience wrapper-script for invoking non-installed ehwe. I.e. the just-built
# binary.

SCRIPT_DIR=$(dirname $(readlink -f $0))
EHWE=$(find "${SCRIPT_DIR}/.." -name ehwe -type f)

# Pick up device from the work-bench
# TBD: Make this smarter when EHWE can control different HW bus-interfaces.
if [ -f "${SCRIPT_DIR}/../src/embedded/.device" ]; then
	source "${SCRIPT_DIR}/../src/embedded/.device"
fi

# Best effort guess defaults in case work-bench doesn't set these
EHWE_DEVICE=${EHWE_DEVICE-"i2c:1:bp:master:/dev/ttyUSB1"}
EHWE_VERBOSITY=${EHWE_VERBOSITY-"warning"}

time $EHWE -d $EHWE_DEVICE -v $EHWE_VERBOSITY -- "${@}"
