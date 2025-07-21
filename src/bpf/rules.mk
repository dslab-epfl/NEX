include mk/subdir_pre.mk

# BPF object files (handled specially in main rules.mk due to special compilation flags)
bpf_sources := $(addprefix $(d), ebpf.c)

# Note: BPF compilation is handled in main rules.mk due to special flags needed
CLEAN += $(d)ebpf.o

include mk/subdir_post.mk
