#!/bin/bash

if [ "$1" = "release" ]
then
    ./build/release/classic-you
else
    ./build/debug/classic-you
fi