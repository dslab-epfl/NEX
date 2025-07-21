# taskset -c 0 python3 -m tvm.exec.rpc_tracker --host 0.0.0.0 --port=9190 &  
# taskset -c 2 python3 -m vta.exec.rpc_server --tracker=0.0.0.0:9190 --key=tsim &
VTA_DEVICE=0 taskset -c 4-8 python3 -m vta.exec.rpc_server &
taskset -c 4-8 python3 deploy_classification-infer.py /home/jiacma/mxnet vta resnet18_v1 cat.png 1 1 0 0
#python3 deploy_classification-infer.py /home/jiacma/mxnet cpu resnet50_v1 cat.png 1 1 0 0
#taskset -c 0-47:2 python3 deploy_classification-infer.py /home/jiacma/mxnet vta resnet50_v1 cat.png 1 1 0 0
#taskset -c 4-8 python3 deploy_classification-infer.py /home/jiacma/mxnet vta resnet50_v1 cat.png 1 1 0 0
