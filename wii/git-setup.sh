#!/bin/bash
chown -R magma:magma /home/magma/game_dev/moonlight-drift-wii
cd /home/magma/game_dev/moonlight-drift-wii
git config --global --add safe.directory /home/magma/game_dev/moonlight-drift-wii
git config user.email "magma@magmacrunch.com"
git config user.name "magma"
git init -b main
git add -A
git status
git commit -m "Initial scaffold: game loop, physics, obstacles, scoring, theme, stars

Phase 1 complete - builds successfully on devkitPPC.
All core modules implemented:
- main.c: 4-state game loop at 60fps
- player.c: thrust/gravity physics
- obstacles.c: generation, movement, tapered collision (3 styles)
- scoring.c: score tracking, SD card JSON persistence
- theme.c: HSL to RGB color generation
- stars.c: 60 blinking/pulsing pixel stars
- renderer.c: console text stub (GRRLIB next)
- input.c: Wiimote WPAD input
- config.h: all game constants"
git remote add origin git@github.com:magmacrunchmedia/moonlight-drift-wii.git
git push -u origin main
