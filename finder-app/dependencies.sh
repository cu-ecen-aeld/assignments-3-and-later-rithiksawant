#!/bin/bash
# Dependency installation script for kernel build.
# Author: Siddhant Jajoo.


sudo apt-get install -y libssl-dev
sudo apt-get install -y u-boot-tools
sudo apt-get install -y qemu

# Had to also install this
sudo apt-get install -y qemu-system-aarch64
