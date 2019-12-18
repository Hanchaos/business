#!/bin/bash

echo "compile project..."
g++ -L../../Depend/lib -I../../Depend/include/librdkafka -lrdkafka -lrdkafka++ ./kafkaProducer.cpp ./kafkaProducerImp.cpp ./main.cpp -o sendKafkaPkg -std=c++11 -g

echo "export LD_LIBRARY_PATH"
CWT_DIR=`pwd`"/../../Depend/lib"

echo "run command below:"
echo "
    export LD_LIBRARY_PATH=\"${CWT_DIR}\"
    export \${LD_LIBRARY_PATH}
    "
