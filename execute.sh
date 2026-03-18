#!/bin/bash

cmake -B build

cd build

rm -rf *

cmake ..

make -j$(sysctl -n hw.ncpu)

sudo -E ./iris_demo
