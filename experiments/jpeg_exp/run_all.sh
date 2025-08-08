#!/bin/bash
CUR_DIR=$(pwd)
mkdir -p $CUR_DIR/../../results/jpeg/
output=$CUR_DIR/../../results/jpeg/
home=$CUR_DIR/../../

make -C $home multi_jpeg_post 

bash ../prepare_config.sh ../configs/jpeg/2_jpeg_dsim
sudo nex $home/multi_jpeg_test_post.out 2 > $output/jpeg_multi_post_2_dsim.log 2>&1
bash ../prepare_config.sh ../configs/jpeg/4_jpeg_dsim
sudo nex $home/multi_jpeg_test_post.out 4 > $output/jpeg_multi_post_4_dsim.log 2>&1
bash ../prepare_config.sh ../configs/jpeg/8_jpeg_dsim
sudo nex $home/multi_jpeg_test_post.out 8 > $output/jpeg_multi_post_8_dsim.log 2>&1


bash ../prepare_config.sh ../configs/jpeg/2_jpeg_rtl
sudo nex $home/multi_jpeg_test_post.out 2 > $output/jpeg_multi_post_2_rtl.log 2>&1

bash ../prepare_config.sh ../configs/jpeg/4_jpeg_rtl
sudo nex $home/multi_jpeg_test_post.out 4 > $output/jpeg_multi_post_4_rtl.log 2>&1

bash ../prepare_config.sh ../configs/jpeg/8_jpeg_rtl
sudo nex $home/multi_jpeg_test_post.out 8 > $output/jpeg_multi_post_8_rtl.log 2>&1


bash ../prepare_config.sh ../configs/jpeg/1_jpeg_legacy_dsim
sudo make -C $home test_single_jpeg > $output/jpeg_seq_dsim_legacy.log 2>&1


bash ../prepare_config.sh ../configs/jpeg/1_jpeg_rtl
sudo make -C $home test_single_jpeg > $output/jpeg_seq_rtl.log 2>&1
