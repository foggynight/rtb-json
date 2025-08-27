#!/bin/sh

for e in $(cat .gitignore); do
    echo "rm: $e"
    rm $e
done
