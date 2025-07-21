sim=$1
latency=$2
rm /tmp/${sim}_sock
./${sim}_simbricks /tmp/${sim}_sock /tmp/${sim}_shm 0 $latency $latency 2000
