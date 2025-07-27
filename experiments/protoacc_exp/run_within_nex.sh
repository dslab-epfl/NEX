#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage: $0 <executable>"
    exit 1
fi

EXECUTABLE=$1

if [ ! -f "$EXECUTABLE" ]; then
    echo "Error: Executable $EXECUTABLE does not exist."
    exit 1
fi

# Run the executable 5 times
for i in {1..4}; do
    echo "Run $i:"
    PROTOACC_DEVICE=0 taskset -c 2 $EXECUTABLE
    echo "---------------------------------"
done

