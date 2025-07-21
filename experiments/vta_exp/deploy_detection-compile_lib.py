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
Compile Library for Pretrained Vision Detection Model
============================================================
**Author**: `Hua Jiang <https://github.com/huajsj>`_
**Modified-By**: `Jonas Kaufmann <https://github.com/jonas-kaufmann>`_

This module is based on the "Deploy Pretrained Vision Detection Model from
Darknet on VTA" tutorial and just compiles the library for the Pretrained Vision
detection Model.
"""

import time
import tvm
import vta
from tvm import autotvm, relay
from tvm.relay.testing.darknet import __darknetffi__
from vta.top import graph_pack
import sys
import json

MODEL_NAME = "yolov3-tiny"


def main():
    if len(sys.argv) != 2:
        print("Usage: deploy_detection-compile_lib.py <darknet_dir>")
        sys.exit(1)

    # Establish all kinds of required files / data
    darknet_dir = sys.argv[1]
    cfg_path = f"{darknet_dir}/{MODEL_NAME}.cfg"
    weights_path = f"{darknet_dir}/{MODEL_NAME}.weights"
    darknet_lib_path = f"{darknet_dir}/libdarknet2.0.so"

    # Load VTA parameters from the 3rdparty/vta-hw/config/vta_config.json file
    env = vta.get_env()
    targets = {
        "vta": env.target,
        "cpu": tvm.target.Target("llvm -mcpu=skylake"),
        "cpu-avx512": tvm.target.Target("llvm -mcpu=skylake-avx512")
    }

    # Name of Darknet model to compile
    # The ``start_pack`` and ``stop_pack`` labels indicate where
    # to start and end the graph packing relay pass: in other words
    # where to start and finish offloading to VTA.
    # the number 4 indicate the ``start_pack`` index is 4, the
    # number 186 indicate the ``stop_pack index`` is 186, by using
    # name and index number, here we can located to correct place
    # where to start/end when there are multiple ``nn.max_pool2d``
    # or ``cast``, print(mod.astext(show_meta_data=False)) can help
    # to find operator name and index information.
    pack_dict = {
        "yolov3-tiny": ["nn.max_pool2d", "cast", 4, 186],
    }
    assert MODEL_NAME in pack_dict

    #####################################
    # Build the inference graph executor.
    # -----------------------------------
    # Using Darknet library load downloaded vision model and compile with Relay.
    # The compilation steps are:
    #
    # 1. Front end translation from Darknet into Relay module.
    # 2. Apply 8-bit quantization: here we skip the first conv layer,
    #    and dense layer which will both be executed in fp32 on the CPU.
    # 3. Perform graph packing to alter the data layout for tensorization.
    # 4. Perform constant folding to reduce number of operators (e.g. eliminate batch norm multiply).
    # 5. Perform relay build to object file.
    # 6. Load the object file onto remote (FPGA device).
    # 7. Generate graph executor, `m`.

    # Load pre-configured AutoTVM schedules
    for target_name, target in targets.items():
        with autotvm.tophub.context(target):
            net = __darknetffi__.dlopen(darknet_lib_path).load_network(
                cfg_path.encode("utf-8"), weights_path.encode("utf-8"), 0
            )
            dshape = (env.BATCH, net.c, net.h, net.w)
            dtype = "float32"

            # Measure build start time
            build_start = time.time()

            # Start front end compilation
            mod, params = relay.frontend.from_darknet(
                net, dtype=dtype, shape=dshape
            )

            if target_name == "vta":
                # Perform quantization in Relay
                # Note: We set opt_level to 3 in order to fold batch norm
                with tvm.transform.PassContext(opt_level=3):
                    with relay.quantize.qconfig(
                        global_scale=23.0,
                        skip_conv_layers=[0],
                        store_lowbit_output=True,
                        round_for_shift=True,
                    ):
                        mod = relay.quantize.quantize(mod, params=params)
                    # Perform graph packing and constant folding for VTA target
                    mod = graph_pack(
                        mod["main"],
                        env.BATCH,
                        env.BLOCK_OUT,
                        env.WGT_WIDTH,
                        start_name=pack_dict[MODEL_NAME][0],
                        stop_name=pack_dict[MODEL_NAME][1],
                        start_name_idx=pack_dict[MODEL_NAME][2],
                        stop_name_idx=pack_dict[MODEL_NAME][3],
                    )
            else:
                mod = mod["main"]

            # Compile Relay program with AlterOpLayout disabled
            with vta.build_config(
                disabled_pass={"AlterOpLayout", "tir.CommonSubexprElimTIR"}
            ):
                lib = relay.build(
                    mod,
                    target=tvm.target.Target(target, host=env.target_host),
                    params=params,
                )

            # Measure Relay build time
            build_time = time.time() - build_start
            print(
                f"{MODEL_NAME} inference graph for {target} built in"
                f" {build_time:.2f}s!"
            )

            # Export the inference library
            lib.export_library(f"{darknet_dir}/graphlib_{target_name}.tar")

            # Export network properties
            net_properties = {"neth": net.h, "netw": net.w}
            with open(
                f"{darknet_dir}/{MODEL_NAME}_properties.json",
                "w",
                encoding="utf-8",
            ) as f:
                json.dump(net_properties, f)


if __name__ == "__main__":
    main()

