#!/bin/bash
awk -F "\t" '{if ($1 != name && $12 == 0) {print}; name=$1}' $1
