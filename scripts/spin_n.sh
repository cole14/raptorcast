#! /bin/bash

N=$1
BASE=9090

for (( i=BASE; i<BASE+N; i++ ))
do
    ./bin/test_client $i 8080 no &
    sleep 0.25
done
