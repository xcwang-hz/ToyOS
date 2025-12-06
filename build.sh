#!/bin/bash

clang --target=wasm32 \
      -nostdlib \
      -Wl,--no-entry \
      -Wl,--export=kernel_entry \
      -Wl,--allow-undefined \
      -o kernel.wasm \
      kernel.cpp    