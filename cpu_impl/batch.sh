#!/bin/bash

for i in {1..60}
do
	srun make NB_THREADS=$i run
	make clean
done
exit 0
