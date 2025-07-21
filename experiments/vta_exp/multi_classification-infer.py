import os
import sys
import time
import random

import numpy as np
import vta
from PIL import Image
import ctypes

import tvm
from tvm.autotvm.measure import measure_methods
from tvm.contrib import graph_executor
import time
from multiprocessing import Process, Array, Barrier
from multiprocessing import shared_memory

def main():
    time.sleep(6)
    if len(sys.argv) != 10:
        print(
            "Usage: deploy_detection-infer.py <mxnet_dir> <target_name>"
            " <model_name> <test_image> <batch_size> <repetitions> <debug> <seed> <num_process>"
        )
        sys.exit(1)

    mxnet_dir = sys.argv[1]
    target_name = sys.argv[2]
    model_name = sys.argv[3]
    test_image = sys.argv[4]
    batch_size = int(sys.argv[5])
    reps = int(sys.argv[6])
    debug = int(sys.argv[7])
    random.seed(int(sys.argv[8]))
    num_process = int(sys.argv[9])

    env = vta.get_env()

    accel_cfg = ""
    if target_name == "vta":
        accel_cfg = f"-{env.BATCH}x{env.BLOCK_OUT}"
    graphlib = f"{mxnet_dir}/graphlib-{model_name}-{target_name}{accel_cfg}.so"
    print(f"Using graphlib: {graphlib}")

    e2e_start = time.time_ns()

    assert (
        batch_size % env.BATCH == 0
    ), f"batch_size={batch_size} env.BATCH={env.BATCH}"

   
    assert tvm.runtime.enabled("rpc")

    image = Image.open(test_image).resize((224, 224))
    image = np.array(image) - np.array([123.0, 117.0, 104.0])
    image /= np.array([58.395, 57.12, 57.375])
    image = image.transpose((2, 0, 1))
    image = image[np.newaxis, :]
    image = np.repeat(image, env.BATCH, axis=0)

    print("Setting bpf_sched to 0x1000")
    shm_name = "/nex_mmio_regions"  # Must include the leading slash
    shm = shared_memory.SharedMemory(name=shm_name)
    buf = shm.buf
    bpf_sched = ctypes.addressof(ctypes.c_char.from_buffer(buf))
    ptr = ctypes.cast(bpf_sched, ctypes.POINTER(ctypes.c_int))
    cid = 999

    def inference_function(start_barrier, end_barrier, dev, reps=1):
        tracker_host = os.environ.get("TVM_TRACKER_HOST", "0.0.0.0")
        tracker_port = 9091
        remote = measure_methods.request_remote(
            env.TARGET, tracker_host, int(tracker_port), timeout=0
        )
        print(f"connected {dev}")
        ctx = remote.ext_dev(0) if target_name == "vta" else remote.cpu(dev)
        remote.upload(graphlib)
        lib = remote.load_module(os.path.basename(graphlib))
        m = graph_executor.GraphModule(lib["default"](ctx))

        start_barrier.wait()
        for i in range(reps):
            print(f"Rep {i} starting inference")
            warmup_start = time.time_ns()
            m.set_input("data", image)
            inference_start = time.time_ns()
            m.run()
            inference_dur = time.time_ns() - inference_start
        
            e2e_dur = time.time_ns() - warmup_start
            print(f"Rep {i}: Total inference duration {e2e_dur:_} ns")
    
        print("done time is", time.time_ns())
        remote._sess.get_function("CloseRPCConnection")()
        end_barrier.wait()

    total = 16
    each = total // num_process
    start_barrier = Barrier(num_process + 1)
    end_barrier = Barrier(num_process + 1)

    p_array = []
    for i in range(num_process):
        p = Process(target=inference_function, args=(start_barrier, end_barrier, i, each))
        p_array.append(p)
        p.start()

    while start_barrier.n_waiting < num_process:
        print(f"{start_barrier.n_waiting} out of {num_process} processes are ready")
        time.sleep(1)

    # @Jonas, the place for gem5 checkpointing? all processses are connected at this point
    print("All processes are ready")
    if cid == 999:
        ptr.contents.value = 0x1000
        nex_lib = ctypes.CDLL('./libnex_tick.so')
        print("Calling tick_nex")
        nex_lib.tick_nex()
        pass
    else:
        pass


    start_barrier.wait()
    real_start = time.clock_gettime(cid)
    time_start = time.time_ns()
    print("start time is", time_start)

    end_barrier.wait()

    time_end = time.time_ns()
    real_end = time.clock_gettime(cid)
    print("end time is", time_end)
    print(f"Time taken: {time_end - time_start}")
    print(f"Real time taken: {real_end - real_start}")

    for p in p_array:
        p.join()

    if not debug:
        return

if __name__ == "__main__":
    main()
