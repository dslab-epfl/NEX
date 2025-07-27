for i in $(seq 0 $(($1-1))); do
    echo "Starting RPC server on VTA device $i (total $1 devices)"
done 