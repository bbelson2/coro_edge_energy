# Windows WSL Ubuntu

## Introduction
The WSL version is designed for analysis and debugging on a Windows machine. It is not designed for running the experiment. The WSL installation is based on Ubuntu 20.04.2 LTS.

## Installing clang

```
sudo apt install clang-12 --install-suggests
```

## Building the source

```
export C=/usr/bin/clang-12
export CXX=/usr/bin/clang++-12
cd ~/repos/coro_edge_energy
mkdir buildl
cd buildl
cmake .. -G "Unix Makefiles"
make
```
