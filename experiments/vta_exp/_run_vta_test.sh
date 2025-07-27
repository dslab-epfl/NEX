VTA_DEVICE=0 taskset -c 2 python3 -m vta.exec.rpc_server &
RPC_SERVER_PID=$!
taskset -c 4 python3 matrix_multiply_opt.py > $1 2>&1
kill -9 $RPC_SERVER_PID
wait