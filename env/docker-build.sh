#!/bin/bash
docker build -t gen_bitstream .
docker save gen_bitstream > gen_bitstream.env.tar
gzip -9 < gen_bitstream.env.tar > gen_bitstream.env.tar.gz
