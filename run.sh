#!/bin/sh
premake5 gmake

cd build
make 
./bin/Debug/minivector $@