#!/bin/bash
cd /home/magma/game_dev/moonlight-drift-wii
cp /mnt/c/Users/magma/game_dev_tmp/main.c source/main.c
cp /mnt/c/Users/magma/game_dev_tmp/config.h source/config.h
cp /mnt/c/Users/magma/game_dev_tmp/Makefile Makefile
export DEVKITPRO=/opt/devkitpro
export DEVKITPPC=/opt/devkitpro/devkitPPC
export PATH=$DEVKITPPC/bin:$PATH
make clean
make
