#!/bin/bash

if [ "$#" == 2 ]; then
	#minimap2 -t 8 -x map-ont -w 10 -d target_ont.mmi $1
	minimap2 -t 32 -x map-ont target_ont.mmi $2 > ont.paf
else
	>&2 echo "Usage: $0 <target.fa> <query.fa>"
	exit 1
fi
