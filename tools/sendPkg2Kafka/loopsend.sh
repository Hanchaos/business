#!/bin/bash

while true
do
 echo "doing"
 ./sendKafkaPkg -c 1 -t yq -f data-ptype32.json
 sleep 0.5
done
