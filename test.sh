#!/bin/sh

gcc -o rtb-json -g rtb-json.c
if [ "$1" == 'g' ]; then
    gdb ./rtb-json
else
    ./rtb-json
fi
