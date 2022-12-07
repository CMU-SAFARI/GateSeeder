#!/bin/bash

make
./aloha_index /hdd/chrom1-2.fna index.ali
#./aloha_index -g /hdd/chrom1-2.fna index_gold.ali

#./aloha_index ../test/dummy.fna debug_index.ali
#./aloha_index -g ../test/dummy.fna debug_index_gold.ali
