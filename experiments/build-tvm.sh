#!/bin/bash
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
