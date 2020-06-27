#! /bin/bash

# Convenience wrapper-script for invoking non-installed ehwe. I.e. the just-built
# binary.

SCRIPT_DIR=$(dirname $(readlink -f $0))
EHWE=$(find "${SCRIPT_DIR}/.." -name ehwe -type f)

# Pick up adapter from the work-bench
# TBD: Make this smarter when EHWE can control different HW bus-interfaces.
if [ -f "${SCRIPT_DIR}/../src/embedded/.adapter" ]; then
	source "${SCRIPT_DIR}/../src/embedded/.adapter"
fi

# Best effort guess defaults in case work-bench doesn't set these
EHWE_ADAPTER=${EHWE_ADAPTER-"i2c:1:bp:master:/dev/ttyUSB1"}
EHWE_VERBOSITY=${EHWE_VERBOSITY-"warning"}

time $EHWE -d $EHWE_ADAPTER -v $EHWE_VERBOSITY -- "${@}"
