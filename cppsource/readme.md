## Libraries

1.  Google Test  
Imported from [GitHub](https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip) using FetchContent. 
1.  FPM  
Header-only fixed-point math library from [GitHub](https://github.com/MikeLankamp/fpm). The 3 headers are copied to `include/fpm`.
1.  TCLAP  
Header-only command-line processing library from [GitHub](https://github.com/mirror/tclap). 


# Raspberry Pi 4 B with 4 GB memory

Distro: Bullseye 32-bit
User: bruce
Set up as Australian English.  

## SSH

1. Raspbery Pi Configuration used to enable SSH  
1. Add PC's public key

  On Pi:

  ```bash
  mkdir .ssh  
  ```

  On Windows:

  ```
  type c:\users\bruce\.ssh\id_rsa.pub | ssh bruce@10.156.55.209 "sudo cat >> ~/.ssh/authorized_keys"  
  ```

## Headless

(Omitted for lite.)

1. Raspbery Pi Configuration (`sudo raspi-config`) > 1 System Options > S5 Boot / Auto Login > B1

## Basic software

```bash
sudo apt-get update
sudo apt-get upgrade
sudo apt-get install build-essential
sudo apt-get install cmake
sudo apt-get install screen
```

Add the following for Bullseye lite:
```bash
sudo apt-get install git
```

## VS Code

1.  Add this line to `/boot/config.txt`:
    `arm_64bit=0`
1.  Reboot Pi  
    `uname -m` should give `armv7l`.
1.  Use VS Code to run `SSH: Uninstall VS Code Server from Host...`
1.  Remote into the Pi from VS on the PC, via Remote Explorer or thru' F1 > SSH > Add New Host...
1.  Close VS Code.
1.  Change the line in `\boot\config.txt` to:
    `arm_64bit=1`
1.  Reboot Pi  
    `uname -m` should give `aarch64`.
1.  Remote into VS.
1.  Install `CMake Tools` extension on Pi
1.  Install `C/C++ Extension Pack` extension on Pi
1.  In Settings > Remote [SSH:...] tab > Extensions > CMake Tools > CMake generator: `Unix Makefiles`


## Git
```bash
git config --global user.name "Bruce Belson"
git config --global user.email "bruce.belson@my.jcu.edu.au"
git config --global pull.rebase false
git config --global credential.helper "cache --timeout=86400"
git config --global advice.addIgnoredFile false
```

## wiringPi

See http://wiringpi.com/wiringpi-updated-to-2-52-for-the-raspberry-pi-4b/

```bash
cd /tmp
wget https://project-downloads.drogon.net/wiringpi-latest.deb
sudo dpkg -i wiringpi-latest.deb
# Check with:
gpio -v
```

The above can 404.

Following https://raspberrypi.stackexchange.com/a/137324 was successful:

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

Testing and exploring the 

```bash
perf_5.10 record -e cache-references,cache-misses,cycles,instructions,branches,faults buildd/infer3_btree -i -v 1 -s 1024 -d 5120 -a 2
perf_5.10 report
```

## Install repo
```bash
cd ~
mkdir repos
cd repos
git clone https://github.com/bbelson2/vanet.git
```
Use a GitHub token in place of password.

# Calibration
```bash
sudo chrt -rr 98 build/gpiocal 100000 100000
```

This sets the priority to real-time, with a priority of -99. The on and off periods will each be 100 microseconds.

Enter 100000 for the waveform count to get a useful span.

# Version 3

## Changes

1.  The sensor id and sequence id become 32-bit.
1.	The datagram width becomes a variable.
1.	Consider removing the first yield in the Version2 coroutine – it’s waiting for the next row in the datagram storage region, which is strictly sequential. The second yield is the useful one – the pointer chaser for the sensor-specific weights.
1.	Consider making the server-side storage size dynamic, driven by the transmitter. (Be careful about side effects, but some throw away early runs should clear these.) Varying the datagram size may be too messy, however – how would the server read the commands?
1.	Simplify the server-side sizing: sensor_count == row_count.

## Outcome

Very small performance enhancements when the inference engine runs on Windows Subsystem for Linux (WSL) - 3% to 8%.  
Negative outcomes on Raspi 4B: -7% to -3%.

Searching the space systematically is hard work, since the transmitter (`transmit`) and the inference engine (`infer`) are on different platforms (WSL and Raspi respectively). We could use command frames to control the inference from the transmitter, but varying the datagram size would add fragility to the receiver. An alternative mechanism of a resident launcher for the inference engine woud add complexity and would distort performance measures.

# Version 4

## infer2

In version 4, we extract the inference portion of the application and run the application locally, using the canned or random transmitted data within the inference application.

This will let us search the parameter space much more efficiently and simply, using simple bash scripts to run the inference application and to analyse the results.

The new application wil be `infer2`. It will contain portions of `transmit` and `infer` but will exclude the UDP code.

## infer3

Next we create a real B+Tree, using the TLX B+Tree and Btree Map classes from [Github](https://github.com/tlx/tlx).

The new application wil be `infer3`. 

A single coroutine will combine the lookup of the weights in the B+Tree and the calculation of the SVM outcome.

An important change here is that there is a yield for every pointer chased by the coroutine. For a B+Tree of depth k this will be at least k+1. This sor of behhaviour is closer to the B+TRee behaviour of chapter 5 (which yielded once for each leaf node).

## TODO

Move cached or writable state value to rt_data?

The single coroutine must be split into a coroutine for the task and a coroutine to look up a key in the B+Tree.

# Version 5

## infer4

To force the cache to flood, we need orders of magnitude more data. 

We will have many lines per sensor - representing different time periods.

Get the data layout from the svm model in Chapter5.

Sensor count will be analogous to batch size in Ch5/SVM. Each task will process the data from a single sensor - many lines of data.

Force cache reload between operations (for test only) - as in Ch5.

## clang 12/LLVM on Pi 4

```
cd ~
wget https://github.com/llvm/llvm-project/releases/tag/llvmorg-12.0.1/clang+llvm-12.0.1-armv7a-linux-gnueabihf.tar.xz
tar -xvf clang+llvm-12.0.1-armv7a-linux-gnueabihf.tar.xz
rm clang+llvm-12.0.1-armv7a-linux-gnueabihf.tar.xz
mv clang+llvm-12.0.1-armv7a-linux-gnueabihf clang_12.0.1
sudo mv clang_12.0.1 /usr/local
```

The wget quit after 162.53 kb on RPI 4 (not 328 MB), so the file was downloaded via the GUI with a browser.

Temporary setup
```
export PATH=/usr/local/clang_12.0.1/bin:$PATH
export LD_LIBRARY_PATH=/usr/local/clang_12.0.1/lib:$LD_LIBRARY_PATH
```
Add these lines to the pi llvm shell scripts?

To run as root, we also need:

```
su
export PATH=/usr/local/clang_12.0.1/bin:$PATH
export LD_LIBRARY_PATH=/usr/local/clang_12.0.1/lib:$LD_LIBRARY_PATH
```

Then simply call directly, e.g.:
```
scripts/explore5a.sh
```

ref https://stackoverflow.com/a/7032021 for build:

```
export PATH=/usr/local/clang_12.0.1/bin:$PATH
export LD_LIBRARY_PATH=/usr/local/clang_12.0.1/lib:$LD_LIBRARY_PATH
export CC=/usr/local/clang_12.0.1/bin/clang
export CXX=/usr/local/clang_12.0.1/bin/clang++
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE="Release" ..
```

## Building on pi

```bash
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE="Release" ..
```

## Running remotely on pi

```
ssh 10.0.0.193 ~/repos/vanet/Version5/scripts/run_infer_l.sh -i -v 1 -s 20 -d 1024 -t 2 -a 8 -c 50
```

## Disable WiFi

Following https://linuxhint.com/disable-wifi-raspberry-pi-through-terminal/

Add the following to an `[all]` section in `/boot/config.txt`

```
dtoverlay=disable-wifi
```

# Version 6

## Remote execution

- Add parameter to delay at start of execution. This should allow the remote caller (Windows) to start listening on the JouleScope. 
- Set JouleScope GPIO3 high at start and low at end of entire sampling period.
- In remote runner, use subprocess.Popen (see https://stackoverflow.com/a/57439663)
- The remote runner does these steps:  
  - Use subprocess.Popen to invoke, e.g., `ssh 192.168.0.101 ~/repos/vanet/Version5/scripts/run_infer_l.sh -b 50 -i -v 1 -s 20 -d 1024 -t 2 -a 8 -c 50`. 
  - On the pi, the script will set the environment variables for LLVM and then start `infer6` detached. 
  - The Windows runner then waits for GPIO3 rising (see `trigger.py` in pyjoulescope_examples).
  - Record V, I, P, GPIO1 & 2 (TODO - to which format?)
  - Stop when GPIO3 falls.
- We should have a unique ID for each run, which will tie together the text outputs on the pi with the recorded JS data on Windows.
- The Python script adds a line containing all the run-time parameters and the unique ID to a database file.

Within the loop we run this:

```
python bin\trigger.py --record --capture_duration 15 --end duration --count 1 --jls_version 1
```

There needs to a delay somewhere to replace the GPIO3 design.

A script will find the newest jls file and rename it.

Set up the pyjoulescope_examples repo as a subtree (not submodule) of our project code.

### Starting up python on Windows

From a conda prompt

```
d:\venv\joulescope\Scripts\activate
```

```
python bin\trigger.py --record --start rising --end falling --count 6 --jls_version 1
```


## Template-ise the iterator driver

Added run_coro.h, containing `coroutine_runner` class.

## Restore some aspects of remote data?  


## Clean up source code

TODO

-  Lose/rewrite the data block description.

## Result

```
python bin\trigger2.py --record --start rising --end falling --count 4 --jls_version 1 --display_stats --event_count 6
ssh 10.0.0.123 ~/repos/vanet/Version6/scripts/run_infer_l_sync.sh -i -v 1 -s 20 -d 1024 -t 2 -a 3 -c 50
```


|Model|Time|Energy|
|---|---|---|
|Coro|2.701ms|7.91006mJ|
|Seq|2.824ms|8.51673mJ|



![results](../docs/spike_pair_edit.png)

# To Do

In Version7:

-  Introduce a count of cache misses (etc)

## perf

Install perf?

```
  WARNING: perf not found for kernel 5.15.90.1-microsoft

  You may need to install the following packages for this specific kernel:
    linux-tools-5.15.90.1-microsoft-standard-WSL2
    linux-cloud-tools-5.15.90.1-microsoft-standard-WSL2

  You may also want to install one of the following packages to keep up to date:
    linux-tools-standard-WSL2
    linux-cloud-tools-standard-WSL2
```    

Following https://gist.github.com/abel0b/b1881e41b9e1c4b16d84e5e083c38a13?permalink_comment_id=4536292#gistcomment-4536292

```
sudo apt update
sudo apt install flex bison
sudo apt install libdwarf-dev libelf-dev libnuma-dev libunwind-dev
libnewt-dev libdwarf++0 libelf++0 libdw-dev libbfb0-dev
systemtap-sdt-dev libssl-dev libperl-dev python-dev-is-python3
binutils-dev libiberty-dev libzstd-dev libcap-dev libbabeltrace-dev
git clone https://github.com/microsoft/WSL2-Linux-Kernel --depth 1
cd WSL2-Linux-Kernel/tools/perf
make -j8 # parallel build
sudo cp perf /usr/local/bin
```

To build on Ubuntu 22:

-  tclap had to update to 1.4 (from the 1.4 branch of the repo - *not the main branch*).
-  small fixes to template class constructors (remove template arguments)


# TO DO

1.  Rebuild the test Raspi 4 (new SD Card)
1.  Install perf 

## Raspberry Pi 4 Rebuild for Version 7

1.  2023-02-21-raspios-bullseye-armhf-lite.img.xz

1.  Instal basic tools

    ```bash
    sudo apt-get update
    sudo apt-get upgrade
    sudo apt-get install build-essential cmake git screen
    ```

1.  Configure Git

    ```bash
    git config --global user.name "Bruce Belson"
    git config --global user.email "bruce.belson@my.jcu.edu.au"
    git config --global pull.rebase false
    git config --global credential.helper "cache --timeout=86400"
    git config --global advice.addIgnoredFile false
    ```
1. WiringPi

    ```bash
    cd /tmp
    wget https://github.com/WiringPi/WiringPi/releases/download/2.61-1/wiringpi-2.61-1-armhf.deb
    sudo dpkg -i wiringpi-2.61-1-armhf.deb
    gpio -v
    ```

1. perf

    There is not yet a perf 5.15 for bullseye, so:

    ```bash
    sudo apt-get install linux-perf-5.10
    sudo nano /usr/bin/perf
    ```

    At line 12, insert:
    ```
    version=5.10
    ```

1. Install repo

    ```bash
    cd ~
    mkdir repos
    cd repos
    git clone https://github.com/bbelson2/vanet.git
    ```

    Use a GitHub token in place of password.

1. Disable WiFi

    Following https://linuxhint.com/disable-wifi-raspberry-pi-through-terminal/

    Add the following to an `[all]` section in `/boot/config.txt`, using `sudo nano`.

    ```
    dtoverlay=disable-wifi
    ```

1.  clang 12/LLVM on Pi 4

    ```
    cd ~
    wget https://github.com/llvm/llvm-project/releases/tag/llvmorg-12.0.1/clang+llvm-12.0.1-armv7a-linux-gnueabihf.tar.xz
    tar -xvf clang+llvm-12.0.1-armv7a-linux-gnueabihf.tar.xz
    rm clang+llvm-12.0.1-armv7a-linux-gnueabihf.tar.xz
    mv clang+llvm-12.0.1-armv7a-linux-gnueabihf clang_12.0.1
    sudo mv clang_12.0.1 /usr/local
    ```

    The wget quit after 162.53 kb on RPI 4
     (not 328 MB), so the file was downloaded via the GUI with a browser, then scp'd over.

  - More clang rtodo

**This rebuild is not required.**

`perf` was already installed and runnable.

# Version 7

## To support perf

- New parameter to `infer7`: -f perf_file
- Passed as GUID by run_infer.py::Experiment
- infer7 must save perf info to report - each line contains count/repeat number and model
- run_infer.py must generate `scp` command line 
- run_infer.py runs `scp` to retrieve perf info (and checks for transfer error?)
- run_infer.py matches perf info to recorded spike data
- When does matching happen?



