# NEX - Hardware Accelerator Full-Stack Simulation Framework

NEX is a simulation framework for running hardware accelerated software full-stack end-to-end. NEX supports unmodified software stack except drivers for accelerators need to be modified slightly for NEX to interpose between software and hardware. 

NEX runs software on the actual CPUs without simulating any CPU architectures. NEX does support simulating more virtual CPUs than what's avaliable.   

NEX supports hardware accelerator simulators in the form of RTL simulator or DSim (LPN-based simulator model). Accelerators including Versatile Tensor Accelerator (VTA), hardware JPEG Decoder, and Protobuf ser./deser. acceleraotr (Protoacc) are integrated into NEX right now. NEX can run multiple of such accelerators as configured. 

NEX supports host-accelerator interconnect modeling, DMA latency modeling as well. 

NEX supports time-warp features that lets users manipulating timestamps in the application. 

## Features

- **Multi-Accelerator Support**: VTA, JPEG decoder, and Protoacc accelerators
- **Dual Simulation Modes**: RTL simulation and fast decoupled functional/performance di-simulation (DSim)
- **Host-Accelerator Interaction**: Modeling of interconnect and memory subsystem
- **Scalable Architecture**: Support for multiple accelerator instances
- **BPF-based Scheduling**: Controlling CPU executing based on [SCX](https://github.com/sched-ext/scx). 

## Architecture

![NEX Architecture](figs/nex-archi.svg)

## Building

### Installing the Kernel
Install kernels that has SCX support [SCX](https://github.com/sched-ext/scx)

For example on Ubuntu 24.04, 

```
$ sudo add-apt-repository -y --enable-source ppa:arighi/sched-ext
$ sudo apt install -y linux-generic-wip scx
$ sudo reboot
```

### Build the NEX repo
Clone the NEX repo.
0. 
```
git submodule update --init --recursive
```
1. make scx 
```
sudo make scx -j
```
2. configure NEX
```
make menuconfig
```
3. make the project. 
```
make -j
make dsim -j
```
4. install NEX so you can it in other directories
```
sudo make install
```

## Configuration

NEX uses a Kconfig-based configuration system. Key configuration options:

### Host Simulation Modes
- **ROUND_BASED_Sim**: Epoch-based CPU scheduling with configurable time slices
- **TOTAL_CORES**: Enter the total number of cores on this system here.
- **SIM_CORES**: Enter the number of cores you want to use for NEX simulation
- **SIM_VIRT_CORES**: Enter the number of virtual cores you want NEX to simulate, leave it 0 if you want every threads runs on a virtual core
- **ROUND_SLICE**: Enter the epoch duration. 
- **EXTRA_COST_TIME**: Adjustment for the epoch duration. If you don't know what to set, try `make autoconfig`, NEX runs a script to find out 
- **DEFAULT_ON_OFF**: If you want to turn the epoch base scheduling on by default or not. Note, you can always turn the scheduling on or off in the application. When the scheduling is on, the application gets a slowdown of 10-20x

### Accelerator Interactive Mode
- **USE_FAULT**: NEX captures host to accelerator communication by segfaults
- **USE_TICK**: NEX captures host to accelerator communication by illegal instructions as ticks (all accelerators integrated into NEX are now using this)
- **EAGER_SYNC**: Turn of eager synchronization or not
- **EAGER_SYNC_PERIOD**: Eager sync period in nanoseconds 

### Accelerator Configuration
- **VTA**: Versatile Tensor Accelerator
- **JPEG**: JPEG Decoder Accelerator
- **Protoacc**: Protoacc Accelerator
- **DSIM**: Functional/performance decoupled simulator based on LPN
- **RTL**: Verilator compiled RTL simulators
- **FREQ**: Frequency of the accelerator
- **LINK_DELAY**: Link delay between the accelerator and host.
- **NUM**: Number of accelerator instances.

### Memory Subsystem
- **MEM_LPN**: memory subsystem simulator based on LPN
- **CACHE_HIT_LATENCY**: Set the cache hit latency in nanoseconds
- **CACHE_MISS_FETCH_LATENCY**: Set the cache miss fetch latency in nanoseconds 
- **CACHE_FIRST_HIT**:  Set whether the first access to an empty cacheline is considered a hit or a miss
- **CACHE_SIZE**: Set cache size in KB.
- **CACHE_ASSOC**: Set cache associativity.

## Usage

You can start NEX simulation simply by running the commands:
```
sudo nex <your application>
```
Note, you want to run multiple commands, you can put all your commands in a script, then run 
```
sudo nex <your script>
```

### Running Prepared Experiments

Prepared experiments are available in `experiments/`:

### VTA
Install env first (note, you might to need to fix env issues mannually, as noted in the build-tvm.sh).
```bash
cd experiments/
./build-tvm.sh
cd vta_exp/
./run_all.sh
``` 
### JPEG
```bash
cd jpeg_exp/
./run_all.sh
```

### Protoacc
```bash 
cd protoacc_exp
./run_all.sh
```

### Results
The results are all in `<NEX>/results`
Running the following will compile the results into python dictionary stored in `results/scripts/compiled_data`
```bash
cd results/scripts
python extract_jpeg.py
python extract_protoacc.py
python extract_vta.py
```

Note, to plot the results in comparison with the gem5 based simulator, you need to copy the compiled results to `results/scripts/gem5_compiled_data`, then run 
```bash
python plot_simtime_speedup.py
```
Note: If your env can't run plot, source the env we created for TVM experiments. Run this command first:`source <NEX-path>/experiments/tvm-vta-env/bin/activate`


## Contact

If you have any questions or suggestions, feel free to reach out to us at (jiacheng.ma@epfl.ch).