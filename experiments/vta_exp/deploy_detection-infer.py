# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
"""
Deploy Pretrained Vision Detection Model from Darknet on VTA
============================================================
**Author**: `Hua Jiang <https://github.com/huajsj>`_
**Modified-By**: `Jonas Kaufmann <https://github.com/jonas-kaufmann>`_

This tutorial provides an end-to-end demo, on how to run Darknet YoloV3-tiny
inference onto the VTA accelerator design to perform Image detection tasks.
It showcases Relay as a front end compiler that can perform quantization (VTA
only supports int8/32 inference) as well as graph packing (in order to enable
tensorization in the core) to massage the compute graph for the hardware target.
"""

import os
import time
import numpy as np
import tvm
from tvm import rpc
from tvm.relay.testing import darknet, yolo_detection
from tvm.autotvm.measure import measure_methods
from tvm.contrib import graph_executor
import tvm.rpc
import vta
import matplotlib.pyplot as plt
import sys
import json
from tvm.relay.testing.darknet import __darknetffi__
import random
import time
import os
import ctypes
import vta.testing.simulator
from multiprocessing import shared_memory

MODEL_NAME = "yolov3-tiny"


def main():
    time.sleep(4)
    if len(sys.argv) != 8:
        print(
            "Usage: deploy_detection-infer.py <darknet_dir> <device>"
            " <test_image> <batch_size> <repetitions> <debug> <seed>"
        )
        sys.exit(1)
    device = sys.argv[2]
    if device not in ["cpu", "vta"]:
        print('Device has to be "cpu" or "vta"')
    test_image = sys.argv[3]
    # batch_size = int(sys.argv[4])
    batch_size = 1
    reps = int(sys.argv[5])
    reps = 1
    debug = int(sys.argv[6])
    random.seed(int(sys.argv[7]))

    # Establish all kinds of required files / data
    darknet_dir = sys.argv[1]
    cfg_path = f"{darknet_dir}/{MODEL_NAME}.cfg"
    weights_path = f"{darknet_dir}/{MODEL_NAME}.weights"
    darknet_lib_path = f"{darknet_dir}/libdarknet2.0.so"
    coco_path = f"{darknet_dir}/coco.names"
    font_path = f"{darknet_dir}/arial.ttf"
    with open(coco_path) as f:
        content = f.readlines()
    names = [x.strip() for x in content]

    # read net properties
    with open(
        f"{darknet_dir}/{MODEL_NAME}_properties.json",
        "r",
        encoding="utf-8",
    ) as f:
        net_properties = json.load(f)
        neth = net_properties["neth"]
        netw = net_properties["netw"]

    # Load VTA parameters from the 3rdparty/vta-hw/config/vta_config.json file
    env = vta.get_env()

    e2e_start = time.time_ns()

    # We run detect on test_image
    img_path = f"{darknet_dir}/{test_image}"
    data = darknet.load_image(img_path, neth, netw).transpose(1, 2, 0)

    # Prepare test image for inference
    data = data.transpose((2, 0, 1))
    data = data[np.newaxis, :]
    data = np.repeat(data, env.BATCH, axis=0)
    assert batch_size % env.BATCH == 0

    nex_lib = ctypes.CDLL('./libnex_tick.so')
    shm_name = "/nex_mmio_regions"  # Must include the leading slash
    shm = shared_memory.SharedMemory(name=shm_name)
    buf = shm.buf
    bpf_sched = ctypes.addressof(ctypes.c_char.from_buffer(buf))

    #######################################################################
    # Connect to tracker or RPC server and request remote inference device.
    # ---------------------------------------------------------------------
    # Tracker handles hands out leases to remote inference devices

    for i in range(reps):
        e2e_start = time.time_ns()
        request_start = time.time_ns()
        '''
        if tracker_host is None or tracker_port is None:
            remote = tvm.rpc.connect(device_host, int(device_port))
        else:
            remote = measure_methods.request_remote(env.TARGET, tracker_host, int(tracker_port), timeout=0)
        '''
        local_session = 0
        if not local_session:
            device_host = os.environ.get("VTA_RPC_HOST", "0.0.0.0")
            device_port = os.environ.get("VTA_RPC_PORT", "9091")
            remote = tvm.rpc.connect(device_host, int(device_port))
        else:
            remote = rpc.LocalSession()

        request_dur = time.time_ns() - request_start

        # Send the inference library over to the remote RPC server
        upload_lib_start = time.time_ns()
        remote.upload(f"{darknet_dir}/graphlib_{device}.tar")
        lib = remote.load_module(f"graphlib_{device}.tar")
        upload_lib_dur = time.time_ns() - upload_lib_start

        # Request remote device
        ctx = remote.ext_dev(0) if device == "vta" else remote.cpu(0)

        m = graph_executor.GraphModule(lib["default"](ctx))
        
        num_inferences = batch_size // env.BATCH
        print(f"Rep {i}: Running {num_inferences} inferences")

        cid = 999
        if cid == 999:
            ptr = ctypes.cast(bpf_sched, ctypes.POINTER(ctypes.c_int))
            ptr.contents.value = 0x1000
            nex_lib.tick_nex()

        else:
            pass            
        real_start = time.clock_gettime(cid)
        # run once, no measurements
        warmup_start = time.time_ns()
        m.set_input("data", data)
        inference_start = time.time_ns()
        
        m.run()
        inference_dur = time.time_ns() - inference_start

        real_end = time.clock_gettime(cid)

        e2e_dur = time.time_ns() - e2e_start
        print(f"Rep {i}: Requesting remote device {request_dur:_} ns")
        print(f"Rep {i}: Sending and loading model {upload_lib_dur:_} ns")
        print(f"Rep {i}: Warmup duration {inference_start - warmup_start:_} ns")
        print(f"Rep {i}: Pure inference duration {inference_dur:_} ns")
        print(f"Rep {i}: Time taken {(inference_dur+inference_start - warmup_start)} ns")
        print(f"Rep {i}: Real latency {real_end-real_start} s")

        for j in range(2):
            warmup_start = time.time_ns()
            m.set_input("data", data)
            inference_start = time.time_ns()
            m.run()
            inference_dur = time.time_ns() - inference_start
            print(f"Rep {j}: Warmup duration {inference_start - warmup_start:_} ns")
            print(f"Rep {j}: Pure inference duration {inference_dur:_} ns")
            print(f"Rep {j}: Time taken {(inference_dur+inference_start - warmup_start)} ns")

        if cid == 999:
            ptr.contents.value = 0x2000
            nex_lib.tick_nex()
            shm.close()

if __name__ == "__main__":
    main()

