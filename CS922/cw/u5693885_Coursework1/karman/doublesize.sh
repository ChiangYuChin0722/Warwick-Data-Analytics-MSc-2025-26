#!/bin/bash

start_imax=660
start_jmax=120
iterations=6

start_cells=$((start_imax * start_jmax))

imax=$start_imax
jmax=$start_jmax

for ((i=0;i<iterations;i++)); do
  cells=$((imax * jmax))
  factor=$((cells / start_cells))

  echo "Iteration $((i+1)): imax=$imax jmax=$jmax cells=$cells (${factor}x initial size)"

  time ./karman --imax="$imax" --jmax="$jmax"
  rm -f karman.bin

  if (( i % 2 == 0 )); then
    imax=$((imax * 2))
  else
    jmax=$((jmax * 2))
  fi
done