# taskset -c 0 python3 -m tvm.exec.rpc_tracker --host 0.0.0.0 --port=9190 & 
# taskset -c 2 python3 -m vta.exec.rpc_server --tracker=0.0.0.0:9190 --key=tsim &
# the above is not used
VTA_DEVICE=0 taskset -c 2 python3 -m vta.exec.rpc_server &
python3 ../simbricks-tvm/vta/tutorials/optimize/matrix_multiply_opt.py
