#!/bin/bash

DIR=$1
OUT=build/sss

cd $DIR
FILES=`ls *`
cd - > /dev/null

mkdir -p $OUT
for TAU in 256 512 2048; do
    for F in $FILES; do
        echo "$F ..."
        build/benchmark/bench_time -a s$TAU -q 1 -r 1 -m r -o out $DIR/$F
        mv sss $OUT/sss_${F}_${TAU}
    done
done
echo "done!"
