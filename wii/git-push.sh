#!/bin/bash
cd /home/magma/game_dev/moonlight-drift-wii
cp /mnt/c/Users/magma/game_dev_tmp/.gitignore .gitignore
git add .gitignore
git rm -r --cached build/
git commit -m "Add .gitignore, remove build artifacts"
git branch -m master main
git push -u origin main
