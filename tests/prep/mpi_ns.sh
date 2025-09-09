#!/usr/bin/env bash
set -euo pipefail

NS0=mpi0; IF0=tap-nccl-0; IP0=10.1.2.1/24
NS1=mpi1; IF1=tap-nccl-1; IP1=10.1.2.2/24
BR=br-mpi; HOST_IP=10.1.2.254/24
HIF0=veth-${NS0}; HIF1=veth-${NS1}

require_root(){ [[ $EUID -eq 0 ]] || { echo "Run as root"; exit 1; }; }
link_exists_host(){ ip -o link show "$1" >/dev/null 2>&1; }
ns_has_link(){ ip netns exec "$1" ip -o link show "$2" >/dev/null 2>&1; }

clean(){
  set +e
  # Try delete taps both in host and namespaces (whichever exist)
  ip link set "$IF0" down 2>/dev/null; ip link del "$IF0" 2>/dev/null
  ip link set "$IF1" down 2>/dev/null; ip link del "$IF1" 2>/dev/null
  ip netns exec "$NS0" ip link del "$IF0" 2>/dev/null
  ip netns exec "$NS1" ip link del "$IF1" 2>/dev/null

  ip link set "$HIF0" down 2>/dev/null; ip link del "$HIF0" 2>/dev/null
  ip link set "$HIF1" down 2>/dev/null; ip link del "$HIF1" 2>/dev/null

  ip addr flush dev "$BR" 2>/dev/null
  ip link set "$BR" down 2>/dev/null; ip link del "$BR" type bridge 2>/dev/null

  ip netns del "$NS0" 2>/dev/null
  ip netns del "$NS1" 2>/dev/null
  echo "Cleaned."
}

fail_if_tap_on_host() {
  for t in "$IF0" "$IF1"; do
    if link_exists_host "$t"; then
      echo "ERROR: $t is on the host (should be inside a namespace). Aborting." >&2
      exit 1
    fi
  done
}

[[ "${1:-}" == "--clean" ]] && { require_root; clean; exit 0; }
require_root
trap 'echo "Failed. Use --clean then retry."; exit 1' ERR

clean || true

# Namespaces
ip netns add "$NS0"
ip netns add "$NS1"

# veth pairs
ip link add "$HIF0" type veth peer name "$IF0"
ip link add "$HIF1" type veth peer name "$IF1"

# Move tap ends into namespaces (as required)
ip link set "$IF0" netns "$NS0"
ip link set "$IF1" netns "$NS1"

# Assert taps are NOT on host; they must be in namespaces
fail_if_tap_on_host
ns_has_link "$NS0" "$IF0" || { echo "ERROR: $IF0 not in $NS0"; exit 1; }
ns_has_link "$NS1" "$IF1" || { echo "ERROR: $IF1 not in $NS1"; exit 1; }

# Bridge on host
ip link add "$BR" type bridge

# Up + enslave
ip link set "$HIF0" up
ip link set "$HIF1" up
ip link set "$BR" up
ip link set "$HIF0" master "$BR"
ip link set "$HIF1" master "$BR"

# Address the bridge
ip addr add "$HOST_IP" dev "$BR"

# Inside namespaces: lo + addr + up (also relax rp_filter just in case)
ip netns exec "$NS0" sh -c "
  ip link set lo up
  ip addr add $IP0 dev $IF0
  ip link set $IF0 up
"
ip netns exec "$NS1" sh -c "
  ip link set lo up
  ip addr add $IP1 dev $IF1
  ip link set $IF1 up
"

echo "Setup complete. Taps are inside namespaces."
echo "Test from host:"
echo "  ping -c2 ${IP0%/*}"
echo "  ping -c2 ${IP1%/*}"
