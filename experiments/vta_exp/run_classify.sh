#!/bin/bash
CUR_DIR=$(pwd)
source $CUR_DIR/../tvm-vta-env/bin/activate
mkdir -p $CUR_DIR/../../results/vta/
common_env="PYTHONPATH=$CUR_DIR/../simbricks-tvm/python:$CUR_DIR/../simbricks-tvm/vta/python env TVM_NUM_THREADS=1 PATH=$PATH"
sudo -E $common_env nex bash _run_classify.sh resnet18_v1 $CUR_DIR/../../results/vta/resnet18_v1.txt
sudo -E $common_env nex bash _run_classify.sh resnet34_v1 $CUR_DIR/../../results/vta/resnet34_v1.txt
sudo -E $common_env nex bash _run_classify.sh resnet50_v1 $CUR_DIR/../../results/vta/resnet50_v1.txt
sudo -E $common_env nex bash _run_detection.sh $CUR_DIR/../../results/vta/yolov3.txt
sudo -E $common_env nex bash _run_vta_test.sh $CUR_DIR/../../results/vta/mm.txt
sudo -E $common_env nex bash multiple_server_single_tracker.sh
