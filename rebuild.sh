#!/bin/bash

./autogen.sh
./configure.sh
rm -f cscope.*
make cscope
cscope -b
make clean all
