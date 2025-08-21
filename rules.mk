include config.mk

CC = gcc
CXX = g++

top_srcdir := $(CONFIG_PROJECT_PATH)
# Initialize legacy libraries variable
LEGACY_LIBS :=
LEGACY_LIB_TARGETS :=
LEGACY_RPATH :=

# No -O3 to avoid deadlock
CFLAGS = -Wall -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function -fPIC -O2 
CXXFLAGS = -Wall -Wno-unused-variable -Wno-unused-but-set-variable -fPIC -O2
LDFLAGS = -ldl -lcapstone -lrt

ifeq ($(CONFIG_ENABLE_BPF), 1)
LDFLAGS += -lbpf
endif

# Include directories
INCLUDE_DIRS = include lib
# src 
INCLUDE = $(foreach dir,$(INCLUDE_DIRS),-I$(dir))

include mk/rules.mk

# Set the default goal to 'all' regardless of target order
.DEFAULT_GOAL := all

# Set directory stack
sp :=
d := ./

# Initialize target-specific object collections
ALL_ALL :=
EXEC_ALL :=
ACCVM_ALL :=
CLEAN_ALL :=
DSIM_ALL :=

EXEC_BINARY := nex
SHARED_LIB := src/accvm.so
BPF_LIB_SO := libmybpf.so

$(eval $(call subdir,src))

# Default target (after subdir processing to populate ALL_ALL)
all: $(EXEC_BINARY) $(SHARED_LIB) $(ALL_ALL)
default: all

# Library paths and dependencies
LIB_SIMBRICKS := lib/libsimbricks.a

# Pre-compiled simbricks libraries (assumed available)
lib_base := lib/simbricks/base/libbase.a
lib_mem := lib/simbricks/mem/libmem.a
lib_pcie := lib/simbricks/pcie/libpcie.a
lib_pciebm := lib/simbricks/pciebm/libpciebm.a

# BPF sources (if enabled)
ifeq ($(CONFIG_ENABLE_BPF), 1)
BPF_OBJECTS := src/bpf/ebpf.o
SCX_BUILD := external/scx/build/

# BPF compilation flags
BPF_CFLAGS := \
	-I$(SCX_BUILD)/scheds/c/scx_simple.p \
	-I$(SCX_BUILD)/scheds/c \
	-I$(SCX_BUILD)/../scheds/c \
	-I$(SCX_BUILD)/../scheds/include \
	-I$(SCX_BUILD)/../build/libbpf/src/usr/include \
	-I$(SCX_BUILD)/../build/libbpf/include/uapi \
	-I./include \
	-fdiagnostics-color=always \
	-D_FILE_OFFSET_BITS=65 \
	-Wall -Winvalid-pch -O0 \
	-fPIC

$(BPF_OBJECTS): %.o : %.c
	$(CC) $(BPF_CFLAGS) -c $< -o $@

# BPF linking (static linking with executable)
BPF_LINKS := -Wl,--start-group \
	$(CURDIR)/external/scx/build/libbpf/src/libbpf.a -lelf -lz -lzstd -Wl,--end-group

$(EXEC_BINARY): $(EXEC_ALL) $(BPF_OBJECTS) $(LEGACY_LIB_TARGETS) config.mk
	$(CXX) -Wall $(INCLUDE) $(EXEC_ALL) $(BPF_OBJECTS) -o $@ -lpthread $(LDFLAGS) \
		-Llib -lsimbricks \
		$(BPF_LINKS) $(LEGACY_LIBS) $(LEGACY_RPATH)
else
$(EXEC_BINARY): $(EXEC_ALL) $(LEGACY_LIB_TARGETS) config.mk
	$(CXX) -Wall  $(INCLUDE) $(EXEC_ALL) -o $@ -lpthread $(LDFLAGS) \
		-Llib -lsimbricks $(LEGACY_LIBS) $(LEGACY_RPATH)
endif

# MMIO interception shared library (uses accvm objects only)
$(SHARED_LIB): $(ACCVM_ALL) config.mk
	$(CC) $(INCLUDE) $(CFLAGS) -shared $(ACCVM_ALL) -o $@ $(LDFLAGS) \
		-Llib -lsimbricks

# SCX scheduler build
scx:
	cd external/scx/ && meson setup --reconfigure build --prefix ~ -Denable_rust=false
	systemctl stop scx.service || true
ifeq ($(CONFIG_STOP_WORLD_MODE), 1)
	cp scx/scx_simple.bpf.ipi.c external/scx/scheds/c/scx_simple.bpf.c
endif
ifeq ($(CONFIG_ROUND_BASED_MODE), 1)
	cp scx/scx_simple.bpf.multiq.c external/scx/scheds/c/scx_simple.bpf.c
endif
	cp scx/scx_simple.c external/scx/scheds/c/scx_simple.c
	meson compile -vC external/scx/build
	meson install -C external/scx/build

# Clean target
clean:
	rm -f $(CLEAN_ALL) $(EXEC_BINARY) $(SHARED_LIB)
	rm -f $(BPF_OBJECTS)

menuconfig:
	@scripts/menuconfig.py

# Install target
install: $(EXEC_BINARY)
	cp $(EXEC_BINARY) /usr/local/bin/$(EXEC_BINARY)
	chmod +x /usr/local/bin/$(EXEC_BINARY)

dsim: $(DSIM_ALL)
	cp ./src/sims/lpn/jpeg_decoder/jpeg_decoder_bm ./simulators/dsim/jpeg
	cp ./src/sims/lpn/vta/vta_bm ./simulators/dsim/vta
	cp ./src/sims/lpn/protoacc/protoacc_bm ./simulators/dsim/protoacc

# Test targets 
test_single_jpeg:
	$(CXX) $(INCLUDE) test/jpeg_decoder_test/application_single_decode.c src/drivers/*.c -o jpeg_test.out
	JPEG_DEVICE=0 ./nex ./jpeg_test.out

multi_jpeg_post:
	$(CXX) -Iinclude -O3 test/jpeg_decoder_test/multiple_device_processing.c src/drivers/*.c -o multi_jpeg_test_post.out -lpthread

test_multi_jpeg_post:
	$(CXX) -Iinclude -O3 test/jpeg_decoder_test/multiple_device_processing.c src/drivers/*.c -o multi_jpeg_test_post.out -lpthread
	./nex ./multi_jpeg_test_post.out 8

compile_matmul:
	$(CXX) -Iinclude -O3 test/nex.matmul.c -o test/nex.matmul

test_matmul:
	@echo "pwd: $(shell pwd)"
	sudo ./nex ./test/nex.matmul

autoconfig:
	$(CXX) -Iinclude -O3 test/nex.matmul.c -o test/nex.matmul
	./test/autoconfig.sh $(CONFIG_PROJECT_PATH) 3 700 1000


autoconfig_vm:
	$(CXX) -Iinclude -O3 test/nex.matmul.c -o test/nex.matmul
	./test/autoconfig.sh $(CONFIG_PROJECT_PATH) 5 2000 3000

.PHONY: all clean scx test_jpeg menuconfig install dsim
