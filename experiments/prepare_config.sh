#!/bin/bash
config_h="$1/config.h"
config_mk="$1/config.mk"
echo "Preparing configuration files from $config_h and $config_mk"
echo "Curdir is $(pwd)"
home=$(pwd)/../..
cp $1/config.h $home/include/config/
cp $1/config.mk $home
make -C $home clean
make -C $home -j
sudo make -C $home install
