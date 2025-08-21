#!/bin/bash
CUR_DIR=$(pwd)
# source $CUR_DIR/../tvm-vta-env/bin/activate
# export TVM_PATH=$PWD/../simbricks-tvm
# export VTA_HW_PATH=$TVM_PATH/3rdparty/vta-hw
# export PYTHONPATH=$TVM_PATH/python:$TVM_PATH/vta/python:$PYTHONPATH
MXNET_HOME=$CUR_DIR/mxnet
mkdir -p $MXNET_HOME
cp -r mxnet_tar/* $MXNET_HOME

# python3 deploy_classification-compile_lib.py vta resnet18_v1 $MXNET_HOME &
# python3 deploy_classification-compile_lib.py vta resnet34_v1 $MXNET_HOME &
# python3 deploy_classification-compile_lib.py vta resnet50_v1 $MXNET_HOME &

# python3 deploy_classification-compile_lib.py cpu resnet18_v1 $MXNET_HOME &
# python3 deploy_classification-compile_lib.py cpu resnet34_v1 $MXNET_HOME &
# python3 deploy_classification-compile_lib.py cpu resnet50_v1 $MXNET_HOME &

