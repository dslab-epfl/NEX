#!/bin/bash

EXECUTABLE=$1

if [ ! -f "$EXECUTABLE" ]; then
    echo "Error: Executable $EXECUTABLE does not exist."
    exit 1
fi

# Run the executable num_repeats times
for i in {1..10}; do
    echo "Run $i:"
    PROTOACC_DEVICE=0 taskset -c 2 $EXECUTABLE
    echo "---------------------------------"
done

