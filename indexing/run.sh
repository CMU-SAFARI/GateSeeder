#!/bin/bash

make
#./demeter_index -w 10 -f 200 -k 15 /mnt/scratch/jeudine/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta /mnt/scratch/jeudine/GRC38_w10_k15_f200.dti
./demeter_index -w 10 -f 200 -k 15 /mnt/scratch/jeudine/chr2-3.fasta /mnt/scratch/jeudine/chr2-3_w10_k15_f200.dti
#./aloha_index -g /mnt/scratch/jeudine/chr2-3.fasta /mnt/scratch/jeudine/chr2-3_gold.dti

#./aloha_index ../test/dummy.fna debug_index.ali
#./aloha_index -g ../test/dummy.fna debug_index_gold.ali
