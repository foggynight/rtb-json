#!/bin/sh

gcc -o test_JSON -g test_JSON.c ../rtb-json.c
g++ -o test_parser -g -std=c++20 test_parser.cpp ../rtb-json.c
