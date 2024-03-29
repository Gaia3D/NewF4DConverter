#!/bin/bash

if [ $# -eq 0 ]; then
	echo "No command was given to run, exiting."
	exit 1
else
	# Start Xvfb
	Xvfb "${DISPLAY}" -ac -screen 0 "${SCREEN}" -nolock -nolisten tcp +extension GLX +render -noreset 2>&1 &
	while ! xdpyinfo -display "${DISPLAY}" > /dev/null 2>&1; do
  		sleep 0.1
	done
	
	# Execute passed command.
	exec "$@"
fi
