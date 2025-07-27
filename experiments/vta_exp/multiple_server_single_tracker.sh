RPC_TRACKER_IP=0.0.0.0
RPC_KEY=simbricks-pci
VTA_RPC_HOST=127.0.0.1
VTA_RPC_PORT=9091

# Array to store background process PIDs
PIDS=()

# server and tracker 
python3 -m tvm.exec.rpc_tracker --host $RPC_TRACKER_IP --port 9091 &
PIDS+=($!)

i=0
while [ $i -lt $1 ]; do
    echo "Launching RPC server on VTA device $i (total $1 devices)"
    VTA_DEVICE=$i python3 -m vta.exec.rpc_server --tracker $RPC_TRACKER_IP:9091 --key $RPC_KEY &
    i=$((i + 1))
    PIDS+=($!)
    sleep 2
done

sleep 10
python3 -m tvm.exec.query_rpc_tracker --host $RPC_TRACKER_IP --port 9091  > $2 2>&1

MXNET_HOME=/tmp/mxnet

taskset -c 0-14:2 python3 multi_classification-infer.py $MXNET_HOME vta resnet18_v1 cat.png 1 1 0 0 $1 >> $2 2>&1

for pid in "${PIDS[@]}"; do
    kill -9 $pid 2>/dev/null
done

wait
