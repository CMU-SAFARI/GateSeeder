#!/bin/bash
awk 'BEGIN { counter = 0 } />/{printf ">%x\n", counter; counter += 1;next}1' $1
