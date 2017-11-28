#!/bin/bash

THIS_WIKI_DIR=$(dirname $(readlink -f $0))

if [ -f ~/.cabal/bin/gitit ]; then
	echo "Will start with local-build gitit binary... (preferred)"
	GITIT_BIN=~/.cabal/bin/gitit
else
	echo "Will start with system installed gitit binary..."
	GITIT_BIN=$(which gitit)
fi

if [ "X${GITIT_BIN}" == "X" ]; then
	echo -n "ERROR: Can't start. No gitit found, either Cabal-built nor " 1>&2
	echo "system installed." 1>&2
	exit 1
fi

if [ "X$(which screen 2>/dev/null)" == "X" ]; then
	echo "Error: screen needed to run service. Please install..."
	exit 1
fi

pushd ${THIS_WIKI_DIR} >/dev/null
	#get config port:
	CPORT=$(grep -e'^port:[[:space:]]' wiki_config.txt | cut -f2 -d" ")
	SESSION_NAME="wiki-$(basename $(cd ../; pwd))"
	echo
	echo "Gitit starting local webserver at http://127.0.0.1:${CPORT} in screen"
	screen -dmS "${SESSION_NAME}" ${GITIT_BIN} -f wiki_config.txt -l 127.0.0.1
	echo "To enter local screen (debugging):"
	echo "  screen -rd \"${SESSION_NAME}\""
	echo
	if	[ "X$(uname -a | grep -i CYGWIN)" != "X" ]; then
		cygstart http://localhost:${CPORT}
	else
		xdg-open http://localhost:${CPORT}
	fi
popd >/dev/null

echo "Opened start-page for you in local browser..."
