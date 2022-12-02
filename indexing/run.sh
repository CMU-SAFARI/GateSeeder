#!/bin/bash

make
./aloha_index /hdd/chrom1-2.fna index.ali
./aloha_index -g /hdd/chrom1-2.fna index_gold.ali
