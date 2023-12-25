#!/bin/sh
echo "\nBuilding the project ..."
echo "==========================\n"

# Run make client command
echo "Building the server program ..."
make server
echo ""

echo "Building the client program ..."
make client
echo ""

# Add more build steps here if needed
echo "=========================="
echo "Build completed.\n"

exit 0
