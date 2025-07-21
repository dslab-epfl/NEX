include mk/subdir_pre.mk

# PCIe simulator objects (compiled directly into executable)
# Source files from perf_sim and func_sim directories
pci_perf_srcs := $(wildcard $(d)perf_sim/*.cc)
pci_func_srcs := $(wildcard $(d)func_sim/*.cc)

# Object files
pci_objs := $(pci_perf_srcs:.cc=.o)
pci_objs += $(pci_func_srcs:.cc=.o)

# Compilation flags
$(pci_objs): CXXFLAGS += -fPIC -O3 -Isrc/sims/common/

# Add objects to executable collection
ALL += $(pci_objs)
EXEC_OBJS += $(pci_objs)
CLEAN += $(pci_objs)

include mk/subdir_post.mk
