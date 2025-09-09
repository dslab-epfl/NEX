#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

### ---- Tunables ----
HOST_IF0=tap-nccl-0
HOST_IF1=tap-nccl-1
IP0=10.1.2.1/24
IP1=10.1.2.2/24
BR=br_ns                 # optional, but good for isolation
### ------------------

echo "== Clean-up =="
ip link del "$HOST_IF0" 2>/dev/null || true
ip link del "$HOST_IF1" 2>/dev/null || true
ip link del "$BR"       2>/dev/null || true
exit 

echo "== Create bridge '$BR' =="
ip link add "$BR" type bridge
ip link set "$BR" up

echo "== Create veth pair =="
ip link add "$HOST_IF0" type veth peer name "$HOST_IF1"

echo "== Attach both ends to bridge =="
ip link set "$HOST_IF0" master "$BR"
ip link set "$HOST_IF1" master "$BR"

echo "== Bring up interfaces =="
ip link set "$HOST_IF0" up
ip link set "$HOST_IF1" up

ip addr add "$IP0" dev "$HOST_IF0"
ip addr add "$IP1" dev "$HOST_IF1"

echo "== Disable GRO/TSO/etc =="
ethtool -K "$HOST_IF0" gro off gso off tso off
ethtool -K "$HOST_IF1" gro off gso off tso off

echo "== Test connectivity =="
ping -c 3 10.1.2.2
ping -c 3 -I "$HOST_IF1" 10.1.2.1

echo "✅ All ready. You can bind to 10.1.2.1 or 10.1.2.2 in your socket app."
