#!/bin/bash

python3 -m venv tvm-vta-env
source tvm-vta-env/bin/activate
pip install -r requirements.txt

cat << 'EOF'
================================================================================
                              TROUBLESHOOTING GUIDE
================================================================================

If you see the error:
    File "..../tvm-vta-env/lib/python3.12/site-packages/mxnet/numpy/utils.py", line 37, in <module>
        bool = onp.bool

Fix: Change to onp.bool_

--------------------------------------------------------------------------------

If you see the error:
    from distutils.version import LooseVersion
    ModuleNotFoundError: No module named 'distutils'

Fix: Change to use:
    from packaging.version import Version
    and change every use of LooseVersion to Version
    similarly if StrictVersion is used, change to use Version as well.

================================================================================
EOF

cd simbricks-tvm
rm -rf build
mkdir build
cp cmake/config.cmake build
cd build
cmake ..
make -j

cd vta_exp
./install_classify.sh
./install_detection.sh
