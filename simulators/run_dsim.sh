sim=$1
latency=$2
rm /tmp/${sim}_dsim_sock
rm /tmp/${sim}_dsim_memsidechannel
./${sim}_bm /tmp/${sim}_dsim_memsidechannel /tmp/${sim}_dsim_memsidechannel_shm /tmp/${sim}_dsim_sock /tmp/${sim}_dsim_shm 0 $latency $latency
