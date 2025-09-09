#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

### —— tunables ——
NS0=ns0; NS1=ns1
BR=br_ns

HOST_IF0=veth-ns0   # host side
HOST_IF1=veth-ns1
NS_IF0=tap-nccl-0   # inside ns0
NS_IF1=tap-nccl-1   # inside ns1

IP0=10.1.2.1/24
IP1=10.1.2.2/24
HOST_IP=10.1.2.254/24
### ————————

echo "🧹  Cleaning previous run"
for ns in $NS0 $NS1;           do ip netns del "$ns"       2>/dev/null || true; done
for i  in $BR $HOST_IF0 $HOST_IF1; do ip link del "$i"     2>/dev/null || true; done

echo "➊  Creating namespaces"
ip netns add "$NS0"
ip netns add "$NS1"

echo "➋  Bridge $BR"
ip link add "$BR" type bridge
ip addr add "$HOST_IP" dev "$BR"
ip link set  "$BR" up

echo "➌  veth for $NS0"
ip link add "$HOST_IF0" type veth peer name "$NS_IF0"
ip link set "$HOST_IF0" master "$BR"
ip link set "$HOST_IF0" up
ip link set "$NS_IF0"   netns "$NS0"

echo "➍  veth for $NS1"
ip link add "$HOST_IF1" type veth peer name "$NS_IF1"
ip link set "$HOST_IF1" master "$BR"
ip link set "$HOST_IF1" up
ip link set "$NS_IF1"   netns "$NS1"

echo "➎  Configure ns0"
ip -n "$NS0" link set lo up
ip -n "$NS0" link set "$NS_IF0" up
ip -n "$NS0" addr add "$IP0" dev "$NS_IF0"
ip netns exec "$NS0" hostname mpi0

echo "➏  Configure ns1"
ip -n "$NS1" link set lo up
ip -n "$NS1" link set "$NS_IF1" up
ip -n "$NS1" addr add "$IP1" dev "$NS_IF1"
ip netns exec "$NS1" hostname mpi1

echo "➐  (Optional) disable offload"
for ifc in "$HOST_IF0" "$HOST_IF1"; do
  ethtool -K "$ifc" gro off gso off tso off  &>/dev/null || true
done
ip netns exec "$NS0" ethtool -K "$NS_IF0" gro off gso off tso off &>/dev/null || true
ip netns exec "$NS1" ethtool -K "$NS_IF1" gro off gso off tso off &>/dev/null || true

echo "➑  Ensure /etc/hosts has node names"
grep -q '^10\.1\.2\.1' /etc/hosts || \
  echo -e "10.1.2.1\tmpi0" | sudo tee -a /etc/hosts
grep -q '^10\.1\.2\.2' /etc/hosts || \
  echo -e "10.1.2.2\tmpi1" | sudo tee -a /etc/hosts

echo -e "\n✅  Network ready:"
ip -br addr show dev "$BR"
ip -n "$NS0" -br addr show dev "$NS_IF0"
ip -n "$NS1" -br addr show dev "$NS_IF1"

cat <<'EOF'

Run-time examples
-----------------
# quick ping check
ip netns exec ns0 ping -c2 10.1.2.2

# minimal 2-rank MPI (needs /usr/sbin/ip with cap_sys_admin or wrapper)
mpirun --host mpi0,mpi1 -np 2 \
  bash -c 'exec ip netns exec ns${OMPI_COMM_WORLD_NODE_RANK} ./mpi_test'

For a hostfile:
  echo -e "mpi0 slots=2\nmpi1 slots=2" > hosts
  mpirun --hostfile hosts -np 4 bash -c 'exec ip netns exec ns${OMPI_COMM_WORLD_NODE_RANK} ./mpi_test'

EOF
