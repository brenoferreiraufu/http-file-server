#!/bin/bash

i=4096
ab -n $i -c $i -s 60 http://127.0.0.1:80/arq2.dat > "./log/test_60_2_$i.txt"
ab -n $i -c $i -s 300 http://127.0.0.1:80/arq2.dat > "./log/test_300_2_$i.txt"
