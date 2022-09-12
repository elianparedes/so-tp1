#!/bin/bash
i=0
while [ "$i" -lt 1000 ]; do
    i=$(( i + 1 ))
    touch "text$i.txt"
    echo $i > "text$i.txt"
done