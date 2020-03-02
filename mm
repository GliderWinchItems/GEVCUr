#!/bin/bash


export FLOAT_TYPE=hard

rm build/GEVCUr.elf
make

./script-all GEVCUr

