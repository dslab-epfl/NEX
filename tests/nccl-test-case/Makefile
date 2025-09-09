#!/bin/bash
 
# Makefile for building MultiDevOneP
# Define compiler
CXX := g++
MPICXX := mpicxx

# Compiler flags
CXXFLAGS := -g -I../../nccl/build/include/ -I/usr/local/cuda/include
MPICXXFLAGS := -g -I../../nccl/build/include/ -I/usr/local/cuda/include

# Source files and targets - non-MPI files
SRC_ONEP := SingleProcessSingleThreadMultipleDevices.c
TARGET_ONEP := SingleProcessSingleThreadMultipleDevices

SRC_TWONODE := TwoNodeOneGPU.c
TARGET_TWONODE := TwoNodeOneGPU

# MPI source files and targets
SRC_MPI_MULTI := MultipleDevicesPerThread.c
TARGET_MPI_MULTI := MultipleDevicesPerThread

SRC_MPI_ONE := OneDevicePerProcess.c
TARGET_MPI_ONE := OneDevicePerProcess

# All sources and targets
SOURCES := $(wildcard *.c)
NON_MPI_TARGETS := $(TARGET_ONEP) $(TARGET_TWONODE)
MPI_TARGETS := $(TARGET_MPI_MULTI) $(TARGET_MPI_ONE)
TARGETS := $(NON_MPI_TARGETS) $(MPI_TARGETS)

# Linker flags
LDFLAGS_ONEP := -O0 -L../../nccl/build/lib -L/usr/local/cuda/lib64 -lnccl -lcudart -lpthread -lrt
MPILDFLAGS := -O0 -L../../nccl/build/lib -L/usr/local/cuda/lib64 -lnccl -lcudart -lpthread -lrt

# Build targets
all: $(TARGETS)

# Non-MPI targets
$(TARGET_ONEP): $(SRC_ONEP)
	$(CXX) $(CXXFLAGS) $< $(LDFLAGS_ONEP) -o $@

$(TARGET_TWONODE): $(SRC_TWONODE)
	$(CXX) $(CXXFLAGS) $< $(LDFLAGS_ONEP) -o $@

# MPI targets
$(TARGET_MPI_MULTI): $(SRC_MPI_MULTI)
	$(MPICXX) $(MPICXXFLAGS) $< $(MPILDFLAGS) -o $@

$(TARGET_MPI_ONE): $(SRC_MPI_ONE)
	$(MPICXX) $(MPICXXFLAGS) $< $(MPILDFLAGS) -o $@

# Clean target
clean:
	rm -f $(TARGETS)

# PHONY targets
.PHONY: all clean run run-twonode run-mpi-multi run-mpi-one run-mpi-one-distributed

# PRELOAD_LIB = "/home/jcm/zhxj-NEX/external/nex-cricket/light/nex_cuda.so"
PRELOAD_LIB = "/home/jcm/nex-cuda-light/light/nex_cuda.so"
# PRELOAD_LIB = ""

PROJECT_DIR := "/home/jcm/nex-cuda-light/"

run: $(TARGET_ONEP)
	@echo "To run with GDB, use: make run GDB=1"
	$(eval CMD=)
	$(if $(GDB),$(eval CMD=gdb --args))
	LD_LIBRARY_PATH="$(PROJECT_DIR)/nccl/build/lib/" \
	LD_PRELOAD=${PRELOAD_LIB}:/home/jcm/nex-cuda-light/nccl/build/lib/libnccl.so:/home/jcm/nex-cuda-light/nccl/ext-net/google-fastsocket/libnccl-net.so:/home/jcm/nex-cuda-light/nccl/ext-profiler/example/libnccl-profiler.so:/home/jcm/nex-cuda-light/nccl/ext-tuner/example/libnccl-tuner.so \
	REPLACE_LIB=${PRELOAD_LIB} \
	NCCL_DEBUG=TRACE \
	NCCL_DEBUG_SUBSYS=ALL \
	NCCL_CROSS_NIC=1 \
	NCCL_TOPO_FILE=$(CURDIR)/topo_2A100_1CX6.xml \
	NCCL_NVLS_ENABLE=0 \
	NCCL_NUM_MOCK_GPU=2 \
	NCCL_NUM_MOCK_NODE=2 \
	$(CMD) ./$(TARGET_ONEP)

run-twonode: $(TARGET_TWONODE)
	@echo "Running two-node NCCL test with TAP interfaces"
	@echo "To run with GDB, use: make run-twonode GDB=1"
	$(eval CMD=)
	$(if $(GDB),$(eval CMD=gdb --args))
	NCCL_DEBUG=TRACE \
	NCCL_DEBUG_SUBSYS=ALL \
	NCCL_CROSS_NIC=1 \
	NCCL_TOPO_DUMP_FILE=$(CURDIR)/topo_dump.xml \
	NCCL_OOB_NET_ENABLE=0 \
	NCCL_P2P_DISABLE=1 \
	NCCL_P2P_LEVEL=LOC \
	NCCL_P2P_DIRECT_DISABLE=1 \
	NCCL_MAX_NCHANNELS=1 \
	NCCL_IB_DISABLE=1 \
	NCCL_SHM_DISABLE=1 \
	NCCL_NVLS_ENABLE=0 \
	NCCL_NUM_MOCK_GPU=2 \
	NCCL_NUM_MOCK_NODE=2 \
	NCCL_NET_SHARED_BUFFERS=0 \
	NCCL_RAS_ENABLE=0\
	$(CMD) ./$(TARGET_TWONODE)
# 	NCCL_NET_GDR_LEVEL=0 \
# LD_LIBRARY_PATH="$(CURDIR)/../../nccl/build/lib/" \
	LD_PRELOAD=${PRELOAD_LIB}:/home/jcm/zhxj-NEX/external/nex-cricket/nccl/build/lib/libnccl.so:/home/jcm/zhxj-NEX/external/nex-cricket/nccl/ext-net/example/libnccl-net.so:/home/jcm/zhxj-NEX/external/nex-cricket/nccl/ext-profiler/example/libnccl-profiler.so:/home/jcm/zhxj-NEX/external/nex-cricket/nccl/ext-tuner/example/libnccl-tuner.so \
	REPLACE_LIB=${PRELOAD_LIB} \
	# NCCL_TOPO_FILE=$(CURDIR)/topo_tap_nccl.xml \

run-mpi-multi: $(TARGET_MPI_MULTI)
	@echo "Running MPI MultipleDevicesPerThread test"
	@echo "To run with GDB, use: make run-mpi-multi GDB=1"
	@echo "Output will be written to mpi-multi-rank-*.out files"
	$(eval CMD=)
	$(if $(GDB),$(eval CMD=gdb --args))
	LD_LIBRARY_PATH="$(CURDIR)/../../nccl/build/lib/" \
	LD_PRELOAD=${PRELOAD_LIB}:/home/jcm/zhxj-NEX/external/nex-cricket/nccl/build/lib/libnccl.so:/home/jcm/zhxj-NEX/external/nex-cricket/nccl/ext-net/example/libnccl-net.so:/home/jcm/zhxj-NEX/external/nex-cricket/nccl/ext-profiler/example/libnccl-profiler.so:/home/jcm/zhxj-NEX/external/nex-cricket/nccl/ext-tuner/example/libnccl-tuner.so \
	REPLACE_LIB=${PRELOAD_LIB} \
	NCCL_DEBUG=TRACE \
	NCCL_DEBUG_SUBSYS=ALL \
	NCCL_CROSS_NIC=1 \
	NCCL_TOPO_FILE=$(CURDIR)/topo_2A100_1CX6.xml \
	NCCL_NVLS_ENABLE=0 \
	NCCL_NUM_MOCK_GPU=2 \
	NCCL_NUM_MOCK_NODE=2 \
	mpirun --output-filename mpi-multi-rank -np 1 -x NCCL_SOCKET_IFNAME=tap-nccl-0 $(CMD) ./$(TARGET_MPI_MULTI) : -np 1 -x NCCL_SOCKET_IFNAME=tap-nccl-1 $(CMD) ./$(TARGET_MPI_MULTI)



COMMAND := 'PORT=$$((12340 + $$OMPI_COMM_WORLD_RANK)); \
            echo "Rank $$OMPI_COMM_WORLD_RANK listening on port $$PORT"; \
            exec gdbserver :$$PORT ./$(TARGET_MPI_ONE)'

# target remote 10.1.2.1:12340
# target remote 10.1.2.2:12341

COMMAND := "./$(TARGET_MPI_ONE)"

LD_LIBRARY_PATH="$(PROJECT_DIR)/nccl/build/lib/" \
LD_PRELOAD=${PRELOAD_LIB}:$(PROJECT_DIR)/nccl/build/lib/libnccl.so:$(PROJECT_DIR)/nccl/ext-profiler/example/libnccl-profiler.so:$(PROJECT_DIR)/nccl/ext-tuner/example/libnccl-tuner.so \

# 	LD_LIBRARY_PATH="$(PROJECT_DIR)/org/nccl/build/lib/" \

# $(PROJECT_DIR)/nccl/ext-net/google-fastsocket/libnccl-net.so
#  	LD_PRELOAD=${PRELOAD_LIB} \
	REPLACE_LIB=${PRELOAD_LIB} \

run-mpi-one: $(TARGET_MPI_ONE)
	NCCL_DEBUG=TRACE \
	NCCL_DEBUG_SUBSYS=ALL \
	NCCL_CROSS_NIC=1 \
	NCCL_TOPO_FILE=$(CURDIR)/topo_tap_nccl.xml \
	NCCL_OOB_NET_ENABLE=0 \
	NCCL_P2P_DISABLE=1 \
	NCCL_CUMEM_ENABLE=0 \
	NCCL_CUMEM_HOST_ENABLE=1 \
	NCCL_P2P_LEVEL=LOC \
	NCCL_COLLNET_ENABLE=0 \
	NCCL_P2P_DIRECT_DISABLE=1 \
	NCCL_MAX_NCHANNELS=1 \
	NCCL_IB_DISABLE=1 \
	NCCL_SHM_DISABLE=1 \
	NCCL_NVLS_ENABLE=0 \
	NCCL_NUM_MOCK_GPU=2 \
	NCCL_NUM_MOCK_NODE=2 \
	NCCL_NET_SHARED_BUFFERS=0 \
	NCCL_RAS_ENABLE=0 \
	NCCL_NET="Socket" \
	NCCL_PROTO="Simple" \
	sudo -E \
	LD_LIBRARY_PATH=$(PROJECT_DIR)/nccl/build/lib/ \
	nex \
	mpirun --allow-run-as-root --hostfile hosts --output-filename mpi-one-rank \
	-np 2 ls > output.log 2>&1

# 	-np 1 \
# 	-x LD_PRELOAD \
# 	-x NCCL_CUMEM_ENABLE \
# 	-x NCCL_CUMEM_HOST_ENABLE \
# 		-x LD_LIBRARY_PATH \
# 		-x REPLACE_LIB \
# 		-x NCCL_DEBUG \
# 		-x NCCL_DEBUG_SUBSYS \
# 		-x NCCL_CROSS_NIC \
# 		-x NCCL_TOPO_FILE \
# 		-x NCCL_OOB_NET_ENABLE \
# 		-x NCCL_P2P_DISABLE \
# 		-x NCCL_P2P_LEVEL \
# 		-x NCCL_COLLNET_ENABLE \
# 		-x NCCL_P2P_DIRECT_DISABLE \
# 		-x NCCL_MAX_NCHANNELS \
# 		-x NCCL_IB_DISABLE \
# 		-x NCCL_SHM_DISABLE \
# 		-x NCCL_NVLS_ENABLE \
# 		-x NCCL_NUM_MOCK_GPU \
# 		-x NCCL_NUM_MOCK_NODE \
# 		-x NCCL_NET_SHARED_BUFFERS \
# 		-x NCCL_RAS_ENABLE \
# 		-x NCCL_NET \
# 		-x NCCL_PROTO \
# 	   -x NCCL_SOCKET_IFNAME=tap-nccl-0 \
# 	   -x NCCL_HOSTID="rs3labsrv865d3961f-75cc-426b-8e7b-3ac85d495be0" \
# 	   -x NCCL_TOPO_DUMP_FILE=$(CURDIR)/topo_dump_mpi0.xml \
# 	   bash -c  $(COMMAND) \
# 	: \
# 	-np 1 \
# 		-x LD_PRELOAD \
# 		-x NCCL_CUMEM_ENABLE \
# 		-x NCCL_CUMEM_HOST_ENABLE \
# 		-x LD_LIBRARY_PATH \
# 		-x REPLACE_LIB \
# 		-x NCCL_DEBUG \
# 		-x NCCL_DEBUG_SUBSYS \
# 		-x NCCL_CROSS_NIC \
# 		-x NCCL_TOPO_FILE \
# 		-x NCCL_OOB_NET_ENABLE \
# 		-x NCCL_P2P_DISABLE \
# 		-x NCCL_P2P_LEVEL \
# 		-x NCCL_COLLNET_ENABLE \
# 		-x NCCL_P2P_DIRECT_DISABLE \
# 		-x NCCL_MAX_NCHANNELS \
# 		-x NCCL_IB_DISABLE \
# 		-x NCCL_SHM_DISABLE \
# 		-x NCCL_NVLS_ENABLE \
# 		-x NCCL_NUM_MOCK_GPU \
# 		-x NCCL_NUM_MOCK_NODE \
# 		-x NCCL_NET_SHARED_BUFFERS \
# 		-x NCCL_RAS_ENABLE \
# 		-x NCCL_NET \
# 		-x NCCL_PROTO \
# 	   -x NCCL_SOCKET_IFNAME=tap-nccl-1 \
# 	   -x NCCL_HOSTID="rs3labsrv831b7e60e-183d-4d26-af0a-fd9a29591648" \
# 	   -x NCCL_TOPO_DUMP_FILE=$(CURDIR)/topo_dump_mpi1.xml \
# 	   bash -c $(COMMAND)


# Variables for distributed execution
REMOTE_HOST ?= icdslab6.epfl.ch
REMOTE_USER ?= zhxj
REMOTE_PATH ?= /home/zhxj/nex-cricket/OneDevicePerProcess

run-mpi-one-distributed: $(TARGET_MPI_ONE)
	@echo "Running MPI OneDevicePerProcess test across two machines"
	@echo "Local machine: localhost, Remote machine: $(REMOTE_HOST)"
	@echo "To run with GDB, use: make run-mpi-one-distributed GDB=1"
	@echo "To specify remote host: make run-mpi-one-distributed REMOTE_HOST=your.remote.host"
	@echo "Output will be written to mpi-one-distributed-*.out files"
	$(eval CMD=)
	$(if $(GDB),$(eval CMD=gdb --args))
	LD_LIBRARY_PATH="$(CURDIR)/../../nccl/build/lib/" \
	LD_PRELOAD=${PRELOAD_LIB}:/home/jcm/zhxj-NEX/external/nex-cricket/nccl/build/lib/libnccl.so:/home/jcm/zhxj-NEX/external/nex-cricket/nccl/ext-net/example/libnccl-net.so:/home/jcm/zhxj-NEX/external/nex-cricket/nccl/ext-profiler/example/libnccl-profiler.so:/home/jcm/zhxj-NEX/external/nex-cricket/nccl/ext-tuner/example/libnccl-tuner.so \
	REPLACE_LIB=${PRELOAD_LIB} \
	NCCL_DEBUG=TRACE \
	NCCL_DEBUG_SUBSYS=ALL \
	NCCL_CROSS_NIC=1 \
	NCCL_TOPO_DUMP_FILE=$(CURDIR)/topo_dump_mpi.xml \
	NCCL_OOB_NET_ENABLE=0 \
	NCCL_P2P_DISABLE=1 \
	NCCL_P2P_LEVEL=LOC \
	NCCL_P2P_DIRECT_DISABLE=1 \
	NCCL_MAX_NCHANNELS=1 \
	NCCL_IB_DISABLE=1 \
	NCCL_SHM_DISABLE=1 \
	NCCL_NVLS_ENABLE=0 \
	NCCL_NUM_MOCK_GPU=2 \
	NCCL_NUM_MOCK_NODE=2 \
	NCCL_NET_SHARED_BUFFERS=0 \
	NCCL_RAS_ENABLE=0 \
	mpirun --output-filename mpi-one-distributed \
	-np 1 -host localhost $(CMD) ./$(TARGET_MPI_ONE) : \
	-np 1 -host $(REMOTE_HOST) $(CMD) $(REMOTE_PATH)/$(TARGET_MPI_ONE)
# 	NCCL_TOPO_FILE=$(CURDIR)/topo_tap_nccl.xml \
