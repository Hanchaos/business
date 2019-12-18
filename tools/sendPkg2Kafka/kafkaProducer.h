#ifndef KAFKA_PRODUCE_H
#define KAFKA_PRODUCE_H

#include <string>
//-----------------------------------------------------

class KafkaProducerImp;

class KafkaProducer {
 public:
  KafkaProducer();
  ~KafkaProducer();

 public:
  bool Init(const std::string broker_list, const std::string topic_name);
  void Uninit();
  bool Produce(const char* data, size_t data_len, const std::string& key);

 private:
  KafkaProducerImp* m_producer_imp;
};
#endif
