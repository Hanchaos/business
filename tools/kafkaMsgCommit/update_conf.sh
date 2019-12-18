#!/bin/bash

cp -rf ../../conf ./

echo "Usage: cd bin && ./kafka_commit"
echo "[NOTE] This command will commit all msgs in kafka, which is specified by conf/xbusiness.conf."
echo "       Program will run in loop, so you need to press \"Ctrl+C\" after you see all msg being committed to exit."

echo -e "\n[ATTENTION!] Please confirm the conf file, especially the topic and broker ip:port from which all msg will be cleared."
cat conf/xbusiness.conf | grep --color topics
cat conf/xbusiness.conf | grep brokers | head -1 | grep --color brokers
