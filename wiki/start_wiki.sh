#!/bin/bash

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

#get config port:
CPORT=$(grep -e'^port:[[:space:]]' wiki_config.txt | cut -f2 -d" ")

if [ "X$(which screen 2>/dev/null)" == "X" ]; then
	echo "Error: screen needed to run service. Please install..."
	exit 1
fi

echo "Gitit starting local webserver at http://127.0.0.1:${CPORT} in screen"
screen -dmS "wiki-$(basename $(pwd))" ${GITIT_BIN} -f wiki_config.txt -l 127.0.0.1
echo "To enter local screen:"
echo "  screen -rd \"wiki-$(basename $(pwd))\""
