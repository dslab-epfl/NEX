#!/bin/bash
CUR_DIR=$(pwd)
mkdir -p $CUR_DIR/../../results/protoacc/
output=$CUR_DIR/../../results/protoacc/

# bash ../prepare_config.sh ../configs/protoacc/protoacc_dsim
# sudo nex bash run_within_nex.sh ./hyperprotobench/nex/binaries/bench0.x86 > $output/bench0-dsim.txt
# sudo nex bash run_within_nex.sh ./hyperprotobench/nex/binaries/bench1.x86 > $output/bench1-dsim.txt
# sudo nex bash run_within_nex.sh ./hyperprotobench/nex/binaries/bench2.x86 > $output/bench2-dsim.txt
# sudo nex bash run_within_nex.sh ./hyperprotobench/nex/binaries/bench3.x86 > $output/bench3-dsim.txt
# sudo nex bash run_within_nex.sh ./hyperprotobench/nex/binaries/bench4.x86 > $output/bench4-dsim.txt  
# sudo nex bash run_within_nex.sh ./hyperprotobench/nex/binaries/bench5.x86 > $output/bench5-dsim.txt

bash ../prepare_config.sh ../configs/protoacc/protoacc_dsim_legacy
sudo nex bash run_within_nex.sh ./hyperprotobench/nex/binaries/bench0.x86 > $output/bench0-legacy-dsim.txt
sudo nex bash run_within_nex.sh ./hyperprotobench/nex/binaries/bench1.x86 > $output/bench1-legacy-dsim.txt
sudo nex bash run_within_nex.sh ./hyperprotobench/nex/binaries/bench2.x86 > $output/bench2-legacy-dsim.txt
sudo nex bash run_within_nex.sh ./hyperprotobench/nex/binaries/bench3.x86 > $output/bench3-legacy-dsim.txt
sudo nex bash run_within_nex.sh ./hyperprotobench/nex/binaries/bench4.x86 > $output/bench4-legacy-dsim.txt
sudo nex bash run_within_nex.sh ./hyperprotobench/nex/binaries/bench5.x86 > $output/bench5-legacy-dsim.txt

bash ../prepare_config.sh ../configs/protoacc/protoacc_rtl
sudo nex bash run_within_nex.sh ./hyperprotobench/nex/binaries/bench0.x86 > $output/bench0-rtl.txt
sudo nex bash run_within_nex.sh ./hyperprotobench/nex/binaries/bench1.x86 > $output/bench1-rtl.txt
sudo nex bash run_within_nex.sh ./hyperprotobench/nex/binaries/bench2.x86 > $output/bench2-rtl.txt
sudo nex bash run_within_nex.sh ./hyperprotobench/nex/binaries/bench3.x86 > $output/bench3-rtl.txt
sudo nex bash run_within_nex.sh ./hyperprotobench/nex/binaries/bench4.x86 > $output/bench4-rtl.txt  
sudo nex bash run_within_nex.sh ./hyperprotobench/nex/binaries/bench5.x86 > $output/bench5-rtl.txt

