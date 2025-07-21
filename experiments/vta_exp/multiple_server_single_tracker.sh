RPC_TRACKER_IP=0.0.0.0
RPC_KEY=simbricks-pci
VTA_RPC_HOST=127.0.0.1
VTA_RPC_PORT=9091

# server and tracker 
taskset -c 0-14:2 python3 -m tvm.exec.rpc_tracker --host $RPC_TRACKER_IP --port 9091 &
VTA_DEVICE=0 taskset -c 0-14:2 python3 -m vta.exec.rpc_server --tracker $RPC_TRACKER_IP:9091 --key $RPC_KEY &
sleep 2
VTA_DEVICE=1 taskset -c 0-14:2 python3 -m vta.exec.rpc_server --tracker $RPC_TRACKER_IP:9091 --key $RPC_KEY &
sleep 2
VTA_DEVICE=2 taskset -c 0-14:2 python3 -m vta.exec.rpc_server --tracker $RPC_TRACKER_IP:9091 --key $RPC_KEY &
sleep 2
VTA_DEVICE=3 taskset -c 0-14:2 python3 -m vta.exec.rpc_server --tracker $RPC_TRACKER_IP:9091 --key $RPC_KEY &
sleep 2
# VTA_DEVICE=4 taskset -c 0-14:2 python3 -m vta.exec.rpc_server --tracker $RPC_TRACKER_IP:9091 --key $RPC_KEY &
# sleep 2
# VTA_DEVICE=5 taskset -c 0-14:2 python3 -m vta.exec.rpc_server --tracker $RPC_TRACKER_IP:9091 --key $RPC_KEY &
# sleep 2
# VTA_DEVICE=6 taskset -c 0-14:2 python3 -m vta.exec.rpc_server --tracker $RPC_TRACKER_IP:9091 --key $RPC_KEY &
# sleep 2
# VTA_DEVICE=7 taskset -c 0-14:2 python3 -m vta.exec.rpc_server --tracker $RPC_TRACKER_IP:9091 --key $RPC_KEY &

sleep 10
python3 -m tvm.exec.query_rpc_tracker --host $RPC_TRACKER_IP --port 9091

MXNET_HOME=/tmp/mxnet

taskset -c 0-14:2 python3 multi_classification-infer.py $MXNET_HOME vta resnet18_v1 cat.png 1 1 0 0 4
