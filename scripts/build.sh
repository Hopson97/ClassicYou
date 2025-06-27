#!/bin/bash

target_release() {
    cd release
    cmake -DCMAKE_BUILD_TYPE=Release ../..
    make
    echo "Built target in build/release/"
    cd ../..
}

target_debug() {
    cd debug 
    cmake -DCMAKE_BUILD_TYPE=Debug ../..
    make
    echo "Built target in build/debug/"
    cd ../..
}

# Create folder for distribution
if [ "$1" = "release" ]
then
    if [ -d "$classic-you" ]
    then
        rm -rf -d classic-you
    fi

    mkdir -p classic-you
fi

# Creates the folder for the buildaries
mkdir -p classic-you 
mkdir -p classic-you/assets
mkdir -p build
mkdir -p build/release
mkdir -p build/debug
cd build

# Builds target
if [ "$1" = "release" ]
then
    target_release
    cp build/release/classic-you classic-you/classic-you
else
    target_debug
fi

cp -R assets classic-you/
