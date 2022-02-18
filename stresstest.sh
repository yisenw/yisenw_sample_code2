#!/bin/bash
shopt -s nullglob
IT=100
APPS=(`find . -name 'test*_app'`)
size=${#APPS[@]}
for ((i=0; i<=IT;i++)); do
    index=$(($RANDOM % $size))
    echo "########## Running ${APPS[$index]}"
    ./${APPS[$index]}
done