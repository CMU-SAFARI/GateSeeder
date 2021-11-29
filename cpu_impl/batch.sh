#!bin/bash

for i in {1..256}
do
	srun -c $i make NB_THREADS=$i run
done
exit 0
