#!/bin/bash

ab -n 2 -c 2 -s 60 http://127.0.0.1:80/arq2.dat
ab -n 4 -c 4 -s 60 http://127.0.0.1:80/arq2.dat
ab -n 8 -c 8 -s 60 http://127.0.0.1:80/arq2.dat
ab -n 16 -c 16 -s 60 http://127.0.0.1:80/arq2.dat
ab -n 32 -c 32 -s 60 http://127.0.0.1:80/arq2.dat
ab -n 64 -c 64 -s 60 http://127.0.0.1:80/arq2.dat
ab -n 128 -c 128 -s 60 http://127.0.0.1:80/arq2.dat
ab -n 256 -c 256 -s 60 http://127.0.0.1:80/arq2.dat
ab -n 512 -c 512 -s 60 http://127.0.0.1:80/arq2.dat
ab -n 1024 -c 1024 -s 60 http://127.0.0.1:80/arq2.dat
ab -n 2048 -c 2048 -s 60 http://127.0.0.1:80/arq2.dat
ab -n 4096 -c 4096 -s 60 http://127.0.0.1:80/arq2.dat


ab -n 2 -c 2 -s 300 http://127.0.0.1:80/arq2.dat
ab -n 4 -c 4 -s 300 http://127.0.0.1:80/arq2.dat
ab -n 8 -c 8 -s 300 http://127.0.0.1:80/arq2.dat
ab -n 16 -c 16 -s 300 http://127.0.0.1:80/arq2.dat
ab -n 32 -c 32 -s 300 http://127.0.0.1:80/arq2.dat
ab -n 64 -c 64 -s 300 http://127.0.0.1:80/arq2.dat
ab -n 128 -c 128 -s 300 http://127.0.0.1:80/arq2.dat
ab -n 256 -c 256 -s 300 http://127.0.0.1:80/arq2.dat
ab -n 512 -c 512 -s 300 http://127.0.0.1:80/arq2.dat
ab -n 1024 -c 1024 -s 300 http://127.0.0.1:80/arq2.dat
ab -n 2048 -c 2048 -s 300 http://127.0.0.1:80/arq2.dat
ab -n 4096 -c 4096 -s 300 http://127.0.0.1:80/arq2.dat
