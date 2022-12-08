#!/bin/bash

make
./aloha_index /hdd/chrom2-3.fna index.ali
./aloha_index -g /hdd/chrom2-3.fna index_gold.ali

#./aloha_index ../test/dummy.fna debug_index.ali
#./aloha_index -g ../test/dummy.fna debug_index_gold.ali
