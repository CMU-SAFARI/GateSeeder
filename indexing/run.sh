#!/bin/bash

make
#./aloha_index /hdd/chrom2-3.fna index.ali
./aloha_index -g /mnt/scratch/jeudine/chr2-3.fasta /mnt/scratch/jeudine/chr2-3_gold.dti

#./aloha_index ../test/dummy.fna debug_index.ali
#./aloha_index -g ../test/dummy.fna debug_index_gold.ali
