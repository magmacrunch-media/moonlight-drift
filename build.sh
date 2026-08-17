#!/bin/bash
export DEVKITPRO=/opt/devkitpro
export DEVKITPPC=/opt/devkitpro/devkitPPC
export PATH=$DEVKITPPC/bin:$PATH
cd /home/magma/game_dev/moonlight-drift-wii
make clean
make
