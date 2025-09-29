#!/bin/bash
echo "USAGE: ./run.sh <nElements>"
echo "------------- COPIAR (ctrl-c) somente a partir da linha abaixo: -----------"
    
for nt in {1..8}
do
    echo "Executando 10 vezes com [$1] elementos e [$nt] threads:"
    for vez in {1..10}  # 10 vezes
    do
        ./prefixSumPth-v1 $1 $nt | grep -oP '(?<=totalTimeInSeconds: )[^ ]*'
    done
done