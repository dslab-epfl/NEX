#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

# ───── tunables ─────
IF0=tap-nccl-0         # first endpoint
IF1=tap-nccl-1         # second endpoint
IP0=10.1.2.1/24        # address on IF0
IP1=10.1.2.2/24        # address on IF1
# ─────────────────────

echo "== Clean-up any leftovers =="
for ifc in "$IF0" "$IF1"; do
    ip link del "$ifc" 2>/dev/null || true
done

echo "== Create veth pair $IF0 ↔ $IF1 =="
ip link add "$IF0" type veth peer name "$IF1"

echo "== Bring both ends up and assign IPs =="
ip addr add "$IP0" dev "$IF0"
ip link set "$IF0" up

ip addr add "$IP1" dev "$IF1"
ip link set "$IF1" up

echo "== Disable segmentation offload (optional, helps packet traces) =="
ethtool -K "$IF0" gro off gso off tso off &>/dev/null || true
ethtool -K "$IF1" gro off gso off tso off &>/dev/null || true

echo -e "\n== Quick self-test =="
ping -c 3 -I "$IF0" "${IP1%/*}"
ping -c 3 -I "$IF1" "${IP0%/*}"

cat <<EOF

💡  Ready to use

•  Any program can now bind to 10.1.2.1 (device $IF0) or 10.1.2.2 (device $IF1)
   exactly like real NICs on the same LAN.

   Example:
     # server
     sudo ./your_server --bind 10.1.2.1
     # client
     sudo ./your_client --connect 10.1.2.2

•  If you ever want to add a third interface (e.g. host address 10.1.2.254),
   you can still create a bridge and enslave both veth ends to it, but it isn’t
   required for two-way traffic between the peers.

EOF
