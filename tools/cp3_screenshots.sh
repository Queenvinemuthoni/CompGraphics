#!/bin/sh
# Renders the CP3 evidence screenshots headlessly (needs xvfb: sudo apt install xvfb).
# Run from the project root after `make engine`.
set -e
mkdir -p outputs/cp3
shot() { name=$1; shift; xvfb-run -a -s '-screen 0 1280x720x24' ./bin/engine "$@" --screenshot "outputs/cp3/$name.ppm" --frames 20; }
shot 18_engine_day       --time 0.15 --pos 7.5 9 23 --look -90 -22 --fly
shot 19_engine_build     --time 0.22 --pos 4.5 6.2 12.5 --look -30 -48 --slot 2
shot 20_engine_sunset    --time 0.48 --pos -4 10 -3 --look 40 -22 --fly
shot 21_engine_night     --time 0.70 --pos 22 11 20 --look -135 -18 --fly
