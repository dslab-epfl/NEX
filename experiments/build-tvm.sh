python3 -m venv tvm-vta-env
source tvm-vta-env/bin/activate
pip install -r requirements.txt
"
simbricks-tvm/python/tvm/_ffi/runtime_ctypes.py", line 98, in DataType
    np.dtype(np.float_): "float64",
"

"
 File "/home/jiacma/new-NEX/experiments/tvm-vta-env/lib/python3.12/site-packages/mxnet/numpy/utils.py", line 37, in <module>
    bool = onp.bool
    change to onp.bool_

 if you see 
from distutils.version import LooseVersion
ModuleNotFoundError: No module named 'distutils' 
change to use 
 from packaging.version import Version
"
cd simbricks-tvm
mkdir build
cp cmake/config.cmake build
cd build
cmake ..
make -j
