# Copyright 2024 Max Planck Institute for Software Systems, and
# National University of Singapore
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

include mk/subdir_pre.mk

# protoacc behavioral model
bin_protoacc_bm := $(d)protoacc_bm

protoacc_bm_objs := $(addprefix $(d), protoacc_bm.o)
protoacc_bm_objs += $(addprefix $(d), func_sim/func_sim.o)
protoacc_bm_objs += $(addprefix $(d), func_sim/lpn_req_map.o)
protoacc_bm_objs += $(addprefix $(d), perf_sim/places.o)

# $(bin_protoacc_bm): CPPFLAGS += -O3 -g  -std=c++17 -Wno-missing-field-initializers
$(bin_protoacc_bm): CPPFLAGS += -O3 -std=c++17 -Wno-missing-field-initializers
$(protoacc_bm_objs): CPPFLAGS += -Isrc -Ilib
$(bin_protoacc_bm): INCLUDE += -I./sims/lpn/protoacc/json_lib
# $(bin_vta_bm): LDFLAGS += -fsanitize=address -static-libasan
$(bin_protoacc_bm): $(protoacc_bm_objs) $(lib_pciebm) $(lib_pcie) $(lib_base) $(lib_mem) $(lib_lpnsim)
	$(CXX) $(CPPFLAGS) $(protoacc_bm_objs) -o $@ $(lib_pciebm) $(lib_pcie) $(lib_base) $(lib_mem) $(lib_lpnsim) $(LDFLAGS) -lpthread

CLEAN := $(bin_protoacc_bm) $(protoacc_bm_objs)

DSIM_ALL += $(bin_protoacc_bm)

include mk/subdir_post.mk
