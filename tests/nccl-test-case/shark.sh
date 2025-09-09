sudo tshark -i veth-ns0 -n   -T fields -e frame.len   > sizes.txt
awk '{sum+=$1} END {print sum}' sizes.txt