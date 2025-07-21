#!/bin/bash
CUR_DIR=$(pwd)
source $CUR_DIR/../tvm-vta-env/bin/activate
sudo -E PYTHONPATH=$CUR_DIR/../simbricks-tvm/python:$CUR_DIR/../simbricks-tvm/vta/python env TVM_NUM_THREADS=1 PATH=$PATH nex bash _run_detection.sh
processes=$(pidof python3)
sudo kill -9 $processes
