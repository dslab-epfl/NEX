VTA_DEVICE=0 taskset -c 2 python3 -m vta.exec.rpc_server &
RPC_SERVER_PID=$!
taskset -c 4 python3 deploy_classification-infer.py $NEX_HOME/experiments/vta_exp/mxnet vta $1 cat.png 1 1 0 0 > $2 2>&1
kill -9 $RPC_SERVER_PID
wait
