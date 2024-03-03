#!/bin/bash

# Filename: bootstrap.sh
# Author: Alan Ramirez Herrera
# Date: 2023-12-12
# Brief: Sets the environment variables for the project

# Set the environment variables
PROJECT_ROOT=$(pwd)
export VCPKG_ROOT=$PROJECT_ROOT/vcpkg
export PATH=$PROJECT_ROOT/vcpkg:$PROJECT_ROOT/scripts:$PATH

