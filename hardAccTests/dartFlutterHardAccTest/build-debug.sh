#!/bin/bash

# Executed on runtime by the flutter-build-debug task, to build the debug
# application. Saves the build on the workspace, therefore caching it for the
# next builds.

echo "Building debug application..."

flutter-elinux pub get
flutter-elinux build elinux --debug
