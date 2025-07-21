include mk/subdir_pre.mk

# vta behavioral model
bin_vta_bm := $(d)vta_bm

vta_bm_objs := $(addprefix $(d), vta_bm.o)
vta_bm_objs += $(addprefix $(d), func_sim/func_sim.o)
vta_bm_objs += $(addprefix $(d), func_sim/lpn_req_map.o)
vta_bm_objs += $(addprefix $(d), perf_sim/places.o)

# $(bin_vta_bm): CPPFLAGS += -O3 -g  -std=c++17 -Wno-missing-field-initializers
$(bin_vta_bm): CPPFLAGS += -O3 -std=c++17 -Wno-missing-field-initializers
$(vta_bm_objs): CPPFLAGS += -Isrc -Ilib
# $(bin_vta_bm): LDFLAGS += -fsanitize=address -static-libasan
$(bin_vta_bm): $(vta_bm_objs) $(lib_pciebm) $(lib_pcie) $(lib_base) $(lib_mem) $(lib_lpnsim)
	$(CXX) $(CPPFLAGS) $(vta_bm_objs) -o $@ $(lib_pciebm) $(lib_pcie) $(lib_base) $(lib_mem) $(lib_lpnsim) $(LDFLAGS) -lpthread

CLEAN := $(bin_vta_bm) $(vta_bm_objs)

ALL := $(bin_vta_bm)

include mk/subdir_post.mk
