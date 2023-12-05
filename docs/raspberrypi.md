# Setting up the Raspbery Pi 

## Hardware - obtain one of:

- Raspberry Pi 4 B with 4 GB memory 
- Raspberry Pi 3 B+ with 1 GB memory

## OS

- Raspberry Pi OS Lite. 
- Distro: Bullseye 32-bit (https://downloads.raspberrypi.com/raspios_oldstable_lite_armhf/images/raspios_oldstable_lite_armhf-2023-10-10/2023-05-03-raspios-bullseye-armhf-lite.img.xz)
- Set up as Australian English
- Enable ssh, and set up a password for a user with the same name as the PC user. Use `~/.ssh/authorized_keys` to set up ssh keys.

### Disable WiFi

Add the following to an `[all]` section in `/boot/config.txt`

```
dtoverlay=disable-wifi
```

## Basic software

```bash
sudo apt-get update
sudo apt-get upgrade
sudo apt-get install build-essential
sudo apt-get install cmake
```

Add the following for Bullseye lite:
```bash
sudo apt-get install git
```

## WiringPi library

Following https://raspberrypi.stackexchange.com/a/137324:

```bash
cd /tmp
wget https://github.com/WiringPi/WiringPi/releases/download/2.61-1/wiringpi-2.61-1-armhf.deb
sudo dpkg -i wiringpi-2.61-1-armhf.deb
gpio -v
```

## perf

```bash
sudo apt-get install linux-perf-5.10
```

## clang

```
cd ~
wget https://github.com/llvm/llvm-project/releases/tag/llvmorg-12.0.1/clang+llvm-12.0.1-armv7a-linux-gnueabihf.tar.xz
tar -xvf clang+llvm-12.0.1-armv7a-linux-gnueabihf.tar.xz
rm clang+llvm-12.0.1-armv7a-linux-gnueabihf.tar.xz
mv clang+llvm-12.0.1-armv7a-linux-gnueabihf clang_12.0.1
sudo mv clang_12.0.1 /usr/local
```

## Install repo
```bash
cd ~
mkdir repos
cd repos
git clone https://github.com/bbelson2/coro_edge_energy.git
```

## Building the C++ source

```
export PATH=/usr/local/clang_12.0.1/bin:$PATH
export LD_LIBRARY_PATH=/usr/local/clang_12.0.1/lib:$LD_LIBRARY_PATH
export CC=/usr/local/clang_12.0.1/bin/clang
export CXX=/usr/local/clang_12.0.1/bin/clang++
cd ~/repos/coro_edge_energy/cppsource
mkdir buildl
cd buildl
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE="Release" ..
make
```

## Running the experiment on the Raspberry Pi

The following script (`run_infer_l_sync.sh`) runs the experiment on the Raspberry Pi. It is run from the `scripts` directory. It passes the command line arguments to the executable `infer7` in the `buildl` directory.

```bash
#!/bin/bash
export PATH=/usr/local/clang_12.0.1/bin:$PATH
export LD_LIBRARY_PATH=/usr/local/clang_12.0.1/lib:$LD_LIBRARY_PATH
~/repos/coro_edge_power/cppsource/buildl/infer7 "$@"
```

This script is invoked remotely from the PC by the experiment runner script, `run_infer7.py`, via the class `Experiment` using commands `create_ssh_cmd_line()` and `run_ssh()`.
