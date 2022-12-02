#!/bin/bash

make
./gold.x ../indexing/index_gold.ali ../test/dummy.fastq 1
