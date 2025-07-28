#!/bin/bash
CUR_DIR=$(pwd)
source $CUR_DIR/../tvm-vta-env/bin/activate
mkdir -p $CUR_DIR/../../results/vta/
common_env="PYTHONPATH=$CUR_DIR/../simbricks-tvm/python:$CUR_DIR/../simbricks-tvm/vta/python env TVM_NUM_THREADS=1 PATH=$PATH"


bash ../prepare_config.sh ../configs/vta/1_vta_dsim_legacy
sudo -E $common_env nex bash _run_classify.sh resnet18_v1 $CUR_DIR/../../results/vta/resnet18_legacy_dsim_v1.txt
sudo -E $common_env nex bash _run_classify.sh resnet34_v1 $CUR_DIR/../../results/vta/resnet34_legacy_dsim_v1.txt
sudo -E $common_env nex bash _run_classify.sh resnet50_v1 $CUR_DIR/../../results/vta/resnet50_legacy_dsim_v1.txt
sudo -E $common_env nex bash _run_detection.sh $CUR_DIR/../../results/vta/yolov3_legacy_dsim.txt
sudo -E $common_env nex bash _run_vta_test.sh $CUR_DIR/../../results/vta/mm_legacy_dsim.txt

bash ../prepare_config.sh ../configs/vta/8_vta_dsim/
sudo -E $common_env nex bash multiple_server_single_tracker.sh 8 $CUR_DIR/../../results/vta/classify_multi-8-resnet18_dsim_v1.txt

bash ../prepare_config.sh ../configs/vta/4_vta_dsim/
sudo -E $common_env nex bash multiple_server_single_tracker.sh 4 $CUR_DIR/../../results/vta/classify_multi-4-resnet18_dsim_v1.txt

bash ../prepare_config.sh ../configs/vta/1_vta_dsim_legacy
sudo -E $common_env nex bash _run_classify.sh resnet18_v1 $CUR_DIR/../../results/vta/resnet18_legacy_dsim_v1.txt
sudo -E $common_env nex bash _run_classify.sh resnet34_v1 $CUR_DIR/../../results/vta/resnet34_legacy_dsim_v1.txt
sudo -E $common_env nex bash _run_classify.sh resnet50_v1 $CUR_DIR/../../results/vta/resnet50_legacy_dsim_v1.txt
sudo -E $common_env nex bash _run_detection.sh $CUR_DIR/../../results/vta/yolov3_legacy_dsim.txt
sudo -E $common_env nex bash _run_vta_test.sh $CUR_DIR/../../results/vta/mm_legacy_dsim.txt


bash ../prepare_config.sh ../configs/vta/4_vta_rtl/
sudo -E $common_env nex bash multiple_server_single_tracker.sh 4 $CUR_DIR/../../results/vta/classify_multi-4-resnet18_rtl_v1.txt
bash ../prepare_config.sh ../configs/vta/8_vta_rtl/
sudo -E $common_env nex bash multiple_server_single_tracker.sh 8 $CUR_DIR/../../results/vta/classify_multi-8-resnet18_rtl_v1.txt


bash ../prepare_config.sh ../configs/vta/1_vta_rtl/
sudo -E $common_env nex bash _run_classify.sh resnet18_v1 $CUR_DIR/../../results/vta/resnet18_rtl_v1.txt
sudo -E $common_env nex bash _run_classify.sh resnet34_v1 $CUR_DIR/../../results/vta/resnet34_rtl_v1.txt
sudo -E $common_env nex bash _run_detection.sh $CUR_DIR/../../results/vta/yolov3_rtl.txt
sudo -E $common_env nex bash _run_vta_test.sh $CUR_DIR/../../results/vta/mm_rtl.txt
sudo -E $common_env nex bash _run_classify.sh resnet50_v1 $CUR_DIR/../../results/vta/resnet50_rtl_v1.txt
