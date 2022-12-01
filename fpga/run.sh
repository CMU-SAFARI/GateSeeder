#!/bin/bash

make
./tb.x  ../indexing/index.ali ../test/dummy.fastq 4
