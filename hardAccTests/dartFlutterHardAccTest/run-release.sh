#!/bin/bash

# Check if the environment variable is set
if [ -z "$APP_PATH" ]; then
  echo "Error: APP_PATH environment variable is not set."
  exit 1
fi

# Set library path
export LD_LIBRARY_PATH=/opt/flutter-elinux/
echo "Running in Release mode: Starting Flutter client..."
/opt/flutter-embedded-linux/build/flutter-client --bundle="$APP_PATH" -f
