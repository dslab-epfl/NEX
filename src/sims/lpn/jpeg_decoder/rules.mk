include mk/subdir_pre.mk

# jpeg decoder behavioral model
bin_jpeg_bm := $(d)jpeg_decoder_bm

jpeg_bm_objs := $(addprefix $(d), jpeg_decoder_bm.o)
jpeg_bm_objs += $(addprefix $(d), func_sim/func_sim.o)
jpeg_bm_objs += $(addprefix $(d), func_sim/lpn_req_map.o)
jpeg_bm_objs += $(addprefix $(d), perf_sim/places.o)

$(bin_jpeg_bm): CPPFLAGS += -O3 -std=c++17 -Wno-missing-field-initializers -Wno-unused-value -Wno-sequence-point
$(jpeg_bm_objs): CPPFLAGS += -I$(d)include -I$(d)../lpn_common -Isrc -Ilib
# $(bin_jpeg_bm): LDFLAGS += -fsanitize=address -static-libasan
$(bin_jpeg_bm): $(jpeg_bm_objs) $(lib_pciebm) $(lib_pcie) $(lib_base) $(lib_mem) $(lib_lpnsim)
	$(CXX) $(CPPFLAGS) $(jpeg_bm_objs) -o $@ $(LDFLAGS) $(lib_pciebm) $(lib_pcie) $(lib_base) $(lib_mem) $(lib_lpnsim) -lpthread

# workload driver
bin_workload_driver := $(d)jpeg_decoder_workload_driver
workload_driver_objs := $(bin_workload_driver).o $(d)vfio.o

# statically linked binary that can run under any Linux image
$(workload_driver_objs): CPPFLAGS += -static
$(bin_workload_driver): LDFLAGS += -static

$(bin_workload_driver): $(workload_driver_objs)
	$(CXX) $(CPPFLAGS) $^ -o $@ $(LDFLAGS)

CLEAN := $(bin_jpeg_bm) $(bin_workload_driver) $(jpeg_bm_objs) \
	$(workload_driver_objs)
ALL := $(bin_jpeg_bm)


include mk/subdir_post.mk
