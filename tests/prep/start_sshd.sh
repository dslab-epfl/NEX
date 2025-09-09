# Setup environment variables for NCCL
# CURDIR=$(pwd)
# NCCL_ENV_VARS="
# export NCCL_DEBUG=TRACE
# export NCCL_DEBUG_SUBSYS=ALL
# export NCCL_CROSS_NIC=1
# export NCCL_TOPO_FILE=$CURDIR/topo_tap_nccl.xml
# export NCCL_OOB_NET_ENABLE=0
# export NCCL_P2P_DISABLE=1
# export NCCL_CUMEM_ENABLE=0
# export NCCL_CUMEM_HOST_ENABLE=1
# export NCCL_P2P_LEVEL=LOC
# export NCCL_COLLNET_ENABLE=0
# export NCCL_P2P_DIRECT_DISABLE=1
# export NCCL_MAX_NCHANNELS=1
# export NCCL_IB_DISABLE=1
# export NCCL_SHM_DISABLE=1
# export NCCL_NVLS_ENABLE=0
# export NCCL_NUM_MOCK_GPU=2
# export NCCL_NUM_MOCK_NODE=2
# export NCCL_NET_SHARED_BUFFERS=0
# export NCCL_RAS_ENABLE=0
# export NCCL_NET=Socket
# export NCCL_PROTO=Simple
# "

# Create the directory in mpi0 with host's mount namespace
nsenter --mount=/proc/1/ns/mnt --net=/var/run/netns/mpi0 \
  mkdir -p /var/run/sshd

# Start sshd in mpi0 with host's mount namespace
nsenter --mount=/proc/1/ns/mnt --net=/var/run/netns/mpi0 \
  /usr/sbin/sshd -o PermitRootLogin=yes -D &

  # Create the directory in mpi0 with host's mount namespace
nsenter --mount=/proc/1/ns/mnt --net=/var/run/netns/mpi1 \
  mkdir -p /var/run/sshd

# Start sshd in mpi0 with host's mount namespace
nsenter --mount=/proc/1/ns/mnt --net=/var/run/netns/mpi1 \
  /usr/sbin/sshd -o PermitRootLogin=yes -D &

exit

# Setup mpi0
# ip netns exec mpi0 apt update
# ip netns exec mpi0 apt install -y openssh-server
ip netns exec mpi0 mkdir -p /var/run/sshd
# echo "$NCCL_ENV_VARS" | ip netns exec mpi0 tee /etc/nccl_env.sh > /dev/null
# ip netns exec mpi0 chmod +x /etc/nccl_env.sh
# echo "source /etc/nccl_env.sh" | ip netns exec mpi0 tee -a /etc/profile > /dev/null
# echo "source /etc/nccl_env.sh" | ip netns exec mpi0 tee -a /etc/bash.bashrc > /dev/null
# echo "$NCCL_ENV_VARS" | ip netns exec mpi0 sed 's/export //g' | ip netns exec mpi0 tee -a /etc/environment > /dev/null
ip netns exec mpi0 /usr/sbin/sshd -o PermitRootLogin=yes -D &

# Setup mpi1
# ip netns exec mpi1 apt update
# ip netns exec mpi1 apt install -y openssh-server
ip netns exec mpi1 mkdir -p /var/run/sshd
# echo "$NCCL_ENV_VARS" | ip netns exec mpi1 tee /etc/nccl_env.sh > /dev/null
# ip netns exec mpi1 chmod +x /etc/nccl_env.sh
# echo "source /etc/nccl_env.sh" | ip netns exec mpi1 tee -a /etc/profile > /dev/null
# echo "source /etc/nccl_env.sh" | ip netns exec mpi1 tee -a /etc/bash.bashrc > /dev/null
# echo "$NCCL_ENV_VARS" | ip netns exec mpi1 sed 's/export //g' | ip netns exec mpi1 tee -a /etc/environment > /dev/null
ip netns exec mpi1 /usr/sbin/sshd -o PermitRootLogin=yes -D &
