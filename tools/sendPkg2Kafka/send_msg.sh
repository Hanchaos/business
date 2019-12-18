#!/bin/bash

TOPIC="tp_business_service"
PKG_FILE="./data.json"
KAFKA_IPPORT="10.10.108.122:9092"

COMMAND="./sendKafkaPkg -c 1 -f $PKG_FILE -t $TOPIC -b $KAFKA_IPPORT"

echo "Running command:"
echo "$COMMAND"
echo "[NOTE] Please pay attention on the TOPIC: [ $TOPIC ] and the kafka ip_port: [ $KAFKA_IPPORT ]"

echo -e "\nResult as below:"
${COMMAND}
