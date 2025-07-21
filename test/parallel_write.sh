for i in `seq 1 1000`; do   
    echo "Running iteration $i"
    ./pure_write.out &
done
wait