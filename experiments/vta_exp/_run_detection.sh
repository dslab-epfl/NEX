#taskset -c 0 python3 -m tvm.exec.rpc_tracker --host 0.0.0.0 --port=9190 &
#taskset -c 2 python3 -m vta.exec.rpc_server --tracker=0.0.0.0:9190 --key=tsim &
VTA_DEVICE=0 taskset -c 2 python3 -m vta.exec.rpc_server &
#taskset -c 4 python3 ./deploy_detection-infer.py /home/jiacma/darknet vta person.jpg 1 1 0 0
python3 ./deploy_detection-infer.py /home/jiacma/darknet vta person.jpg 1 1 0 0
