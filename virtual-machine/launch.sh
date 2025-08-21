qemu-system-x86_64  \
  -machine accel=kvm,type=q35 \
  -smp 24 \
  -cpu host \
  -m 24G \
  -nographic \
  -device virtio-net-pci,netdev=net0 \
  -netdev user,id=net0,hostfwd=tcp::2222-:22 \
  -drive if=virtio,format=qcow2,file=noble-server-cloudimg-amd64.img \
  -drive if=virtio,format=raw,file=seed.img
