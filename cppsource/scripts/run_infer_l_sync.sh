#!/bin/bash

export PATH=/usr/local/clang_12.0.1/bin:$PATH
export LD_LIBRARY_PATH=/usr/local/clang_12.0.1/lib:$LD_LIBRARY_PATH
~/repos/coro_edge_energy/cppsource/buildl/infer7 "$@"
