#ifndef KAFKA_CONSUMER_IMP_CB_H
#define KAFKA_CONSUMER_IMP_CB_H
#include "librdkafka/rdkafkacpp.h"
#include <iostream>
#include <stdio.h>

class MsgConsumer;

class ExampleEventCb : public RdKafka::EventCb {
public:
    void event_cb(RdKafka::Event &event);
};

class ExampleRebalanceCb : public RdKafka::RebalanceCb {
 private:
  static void part_list_print(const std::vector<RdKafka::TopicPartition *> &partitions);
 public:
  void rebalance_cb(RdKafka::KafkaConsumer *consumer, RdKafka::ErrorCode err,std::vector<RdKafka::TopicPartition *> &partitions); 
};

#endif