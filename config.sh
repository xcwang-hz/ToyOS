#!/bin/bash

BUILD_TYPE="Release"

# If -d is passed, switch to Debug mode
while getopts "d" opt; do
  case $opt in
    d)
      echo ">> [INFO] Debug mode enabled (-g -O0)"
      BUILD_TYPE="Debug"
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      exit 1
      ;;
  esac
done

# Create build directory
mkdir -p build
cd build

# Run CMake with the specified build type
echo ">> Configuring project with type: $BUILD_TYPE"
CC=clang CXX=clang++ cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE