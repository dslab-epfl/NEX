#!/bin/bash
# Save current CONFIG_DEFAULT_ON_OFF value before changing it
ProjectPath=$1
PERCENTAGE=$2
LB=$3
UB=$4
# Get baseline measurement (without BPF)
echo "Getting baseline measurement..."
for i in {1..3}; do
    $ProjectPath/test/nex.matmul > /dev/null 2>&1
done
baseline=$(taskset -c 0 $ProjectPath/test/nex.matmul | grep -o '[0-9]\+' | head -1)
echo "Baseline: $baseline"

original_on_off=$(grep "#define CONFIG_DEFAULT_ON_OFF" $ProjectPath/include/config/config.h | grep -o '[0-9]\+')
sed -i "s/#define CONFIG_DEFAULT_ON_OFF [0-9]/#define CONFIG_DEFAULT_ON_OFF 1/" $ProjectPath/include/config/config.h

# sed -i "s/#define CONFIG_DEFAULT_ON_OFF (0-9)/#define CONFIG_DEFAULT_ON_OFF 1/" $ProjectPath/include/config/config.h
# for NEW_VALUE in $(seq $LB 50 $UB); do
for NEW_VALUE in $(seq $UB -50 $LB); do
    echo "Setting CONFIG_EXTRA_COST_TIME to $NEW_VALUE"
    sed -i "s/#define CONFIG_EXTRA_COST_TIME [0-9]\+/#define CONFIG_EXTRA_COST_TIME $NEW_VALUE/" $ProjectPath/include/config/config.h
    sed -i "s/CONFIG_EXTRA_COST_TIME=[0-9]\+/CONFIG_EXTRA_COST_TIME=$NEW_VALUE/" $ProjectPath/config.mk
    sed -i "s/CONFIG_EXTRA_COST_TIME=[0-9]\+/CONFIG_EXTRA_COST_TIME=$NEW_VALUE/" $ProjectPath/.config
    
    make -C $ProjectPath -B -j > /dev/null 2>&1
    
    make -C $ProjectPath test_matmul
    # Get the measurement with current NEW_VALUE
    measurement=$(make -C $ProjectPath test_matmul | grep -oP 'Elapsed time: \K[0-9]+' | head -1)

    echo "Measurement with $NEW_VALUE: $measurement"
    
    # Compare values (within 10% tolerance)
    diff=$((measurement - baseline))
    if [ $diff -lt 0 ]; then
        diff=$((-diff))
    fi

    tolerance=$((baseline / (100/$PERCENTAGE)))  # tolerance

    if [ $diff -le $tolerance ]; then
        echo "FOUND OPTIMAL VALUE: $NEW_VALUE (diff: $diff, tolerance: $tolerance)"
        # Revert CONFIG_DEFAULT_ON_OFF before exit
        sed -i "s/#define CONFIG_DEFAULT_ON_OFF [0-9]/#define CONFIG_DEFAULT_ON_OFF $original_on_off/" $ProjectPath/include/config/config.h
        exit 0
    else
        echo "Difference too large: $diff (tolerance: $tolerance)"
    fi
done

echo "No value found within range $LB to $UB"
#revert changes
sed -i "s/#define CONFIG_EXTRA_COST_TIME [0-9]*/#define CONFIG_EXTRA_COST_TIME 0/" $ProjectPath/include/config/config.h
sed -i "s/CONFIG_EXTRA_COST_TIME=[0-9]*/CONFIG_EXTRA_COST_TIME=0/" $ProjectPath/config.mk
sed -i "s/CONFIG_EXTRA_COST_TIME=[0-9]*/CONFIG_EXTRA_COST_TIME=0/" $ProjectPath/.config
sed -i "s/#define CONFIG_DEFAULT_ON_OFF [0-9]/#define CONFIG_DEFAULT_ON_OFF $original_on_off/" $ProjectPath/include/config/config.h
make -C $ProjectPath