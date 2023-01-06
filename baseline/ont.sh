#!/bin/bash

if [ "$#" == 2 ]; then
	#minimap2 -t 32 -x map-ont -d /local/home/jeudine/target_ont.mmi $1
	minimap2 -t 32 -x map-ont /local/home/jeudine/target_ont.mmi $2 > /local/home/jeudine/ont.paf
else
	>&2 echo "Usage: $0 <target.fa> <query.fa>"
	exit 1
fi
