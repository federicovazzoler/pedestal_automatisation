#!/bin/bash

root -l -b -q utility/checkTree.C >> out.log

cat out.log | grep -v Checking

if [ $? -ne 0 ]; then
    echo "[ERROR]: The rootple is corrupted! Maybe you can re-do the hadd of all files."
    mv out.log ERROR.log
else
    echo "[INFO]: The rootple is fine and ready to be submitted."
    rm out.log
fi
