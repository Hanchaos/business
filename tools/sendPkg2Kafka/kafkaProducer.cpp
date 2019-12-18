#include "kafkaProducer.h"

#include <strings.h>

#include <set>
#include <map>
#include <iostream>
#include "rdkafka.h"
#include "kafkaProducerImp.h"

extern void log(std::string s);

KafkaProducer::KafkaProducer() { m_producer_imp = new KafkaProducerImp(); }

KafkaProducer::~KafkaProducer() {
  if (NULL != m_producer_imp) {
    delete m_producer_imp;
    m_producer_imp = NULL;
  }
}

bool KafkaProducer::Init(const std::string broker_list,
                         const std::string topic_name) {
  bool rt = false;

  if (NULL != m_producer_imp) {
    rt = m_producer_imp->Init(broker_list, topic_name);
  }

  return true;
}

void KafkaProducer::Uninit() {
  if (NULL != m_producer_imp) {
    m_producer_imp->Uninit();
  }
}

bool KafkaProducer::Produce(const char* data, size_t data_len,
                            const std::string& key) {
  bool rt = true;
  if (NULL != data && data_len > 0 && NULL != m_producer_imp) {
    rt = m_producer_imp->Produce(data, data_len, key);
  } else {
    log("KafkaProducer::produce fail to producer");
  }

  return rt;
}
