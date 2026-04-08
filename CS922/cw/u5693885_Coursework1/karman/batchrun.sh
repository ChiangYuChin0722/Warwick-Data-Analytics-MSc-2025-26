#!/bin/bash

# Loop through a range of values (e.g., 1 to 5)
for i in {1..20}
do
    echo "Running karman simulation $i"
    ./karman
    
    # convert and save output for this run
    ./bin2ppm -i karman.bin -o Run$i.ppm
        
done

echo "All simulations completed"