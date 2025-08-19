#!/bin/sh

g++ -o test_cases -std=c++20 test_cases.cpp ../rtb-json.c
g++ -o test_input test_input.cpp ../rtb-json.c
