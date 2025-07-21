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
Deploy Pretrained Vision Model from MxNet on VTA
================================================
**Author**: `Thierry Moreau <https://homes.cs.washington.edu/~moreau/>`_

This tutorial provides an end-to-end demo, on how to run ImageNet classification
inference onto the VTA accelerator design to perform ImageNet classification tasks.
It showcases Relay as a front end compiler that can perform quantization (VTA
only supports int8/32 inference) as well as graph packing (in order to enable
tensorization in the core) to massage the compute graph for the hardware target.
"""

import sys
import time

import vta
from mxnet.gluon.model_zoo import vision
from vta.top import graph_pack

import tvm
from tvm import autotvm, relay


def main():
    if len(sys.argv) != 4:
        print(
            "Usage: deploy_classification-compile_lib.py <target_name>"
            " <model_name> <output_dir>"
        )
        sys.exit(1)

    target_name = sys.argv[1]
    model_name = sys.argv[2]
    output_dir = sys.argv[3]

    # Load VTA parameters from the 3rdparty/vta-hw/config/vta_config.json file
    env = vta.get_env()
    targets = {
        "vta": env.target,
        "cpu": tvm.target.Target("llvm -mcpu=skylake"),
        "cpu_avx512": tvm.target.Target("llvm -mcpu=skylake-avx512"),
    }
    target = targets[target_name]

    # Dictionary lookup for when to start/end bit packing
    pack_dict = {
        "resnet18_v1": ["nn.max_pool2d", "nn.global_avg_pool2d"],
        "resnet34_v1": ["nn.max_pool2d", "nn.global_avg_pool2d"],
        "resnet50_v1": ["nn.max_pool2d", "nn.global_avg_pool2d"],
        "resnet101_v1": ["nn.max_pool2d", "nn.global_avg_pool2d"],
        "resnet18_v2": ["nn.max_pool2d", "nn.global_avg_pool2d"],
        "resnet34_v2": ["nn.max_pool2d", "nn.global_avg_pool2d"],
        "resnet50_v2": ["nn.max_pool2d", "nn.global_avg_pool2d"],
        "resnet101_v2": ["nn.max_pool2d", "nn.global_avg_pool2d"],
    }

    # Name of Gluon model to compile
    # The ``start_pack`` and ``stop_pack`` labels indicate where
    # to start and end the graph packing relay pass: in other words
    # where to start and finish offloading to VTA.
    assert model_name in pack_dict

    ######################################################################
    # Build the inference graph executor
    # ----------------------------------
    # Grab vision model from Gluon model zoo and compile with Relay.
    # The compilation steps are:
    #
    # 1. Front end translation from MxNet into Relay module.
    # 2. Apply 8-bit quantization: here we skip the first conv layer,
    #    and dense layer which will both be executed in fp32 on the CPU.
    # 3. Perform graph packing to alter the data layout for tensorization.
    # 4. Perform constant folding to reduce number of operators (e.g. eliminate batch norm multiply).
    # 5. Perform relay build to object file.
    # 6. Load the object file onto remote (FPGA device).
    # 7. Generate graph executor, `m`.

    # Load pre-configured AutoTVM schedules
    with autotvm.tophub.context(target):

        # Populate the shape and data type dictionary for ImageNet classifier input
        dtype_dict = {"data": "float32"}
        shape_dict = {"data": (env.BATCH, 3, 224, 224)}

        # Get off the shelf gluon model, and convert to relay
        gluon_model = vision.get_model(model_name, pretrained=True)

        # Measure build start time
        build_start = time.time()

        # Start front end compilation
        mod, params = relay.frontend.from_mxnet(gluon_model, shape_dict)

        # Update shape and type dictionary
        shape_dict.update({k: v.shape for k, v in params.items()})
        dtype_dict.update({k: str(v.dtype) for k, v in params.items()})

        if target.device_name == "vta":
            # Perform quantization in Relay
            # Note: We set opt_level to 3 in order to fold batch norm
            with tvm.transform.PassContext(opt_level=3):
                with relay.quantize.qconfig(
                    global_scale=8.0, skip_conv_layers=[0]
                ):
                    mod = relay.quantize.quantize(mod, params=params)
                # Perform graph packing and constant folding for VTA target
                assert env.BLOCK_IN == env.BLOCK_OUT
                print(f"Building for batch={env.BATCH} block={env.BLOCK_OUT}")
                relay_prog = graph_pack(
                    mod["main"],
                    env.BATCH,
                    env.BLOCK_OUT,
                    env.WGT_WIDTH,
                    start_name=pack_dict[model_name][0],
                    stop_name=pack_dict[model_name][1],
                )
        else:
            relay_prog = mod["main"]

        # Compile Relay program with AlterOpLayout disabled
        if target.device_name != "vta":
            with tvm.transform.PassContext(
                opt_level=3, disabled_pass={"AlterOpLayout"}
            ):
                lib = relay.build(
                    relay_prog,
                    target=tvm.target.Target(target, host=env.target_host),
                    params=params,
                )
        else:
            with vta.build_config(
                opt_level=3,
                disabled_pass={"AlterOpLayout", "tir.CommonSubexprElimTIR"},
            ):
                lib = relay.build(
                    relay_prog,
                    target=tvm.target.Target(target, host=env.target_host),
                    params=params,
                )

        # Measure Relay build time
        build_time = time.time() - build_start
        print(
            model_name
            + " inference graph built in {0:.2f}s!".format(build_time)
        )

        # Export the inference library
        accel_cfg = ""
        if target_name == "vta":
            accel_cfg = f"-{env.BATCH}x{env.BLOCK_OUT}"
        lib.export_library(
            f"{output_dir}/graphlib-{model_name}-{target_name}{accel_cfg}.so"
        )


if __name__ == "__main__":
    main()

