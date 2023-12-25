#!/bin/sh
echo "\nCleaning the project ..."
echo "=========================="
make clean
echo "clean completed."
./build.sh
exit 0