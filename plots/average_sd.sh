#!/bin/bash
awk '{m1 += $1 ;m2 += $1**2; n++} END {if (n > 0) {av = m1/n; print av; print sqrt(m2 / n - av**2)}}' $1
