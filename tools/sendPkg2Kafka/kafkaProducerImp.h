#ifndef KAFKA_PRODUCER_IMP_H
#define KAFKA_PRODUCER_IMP_H

#include <string>

#include "rdkafka.h"

class KafkaProducerImp {
 public:
  KafkaProducerImp();
  ~KafkaProducerImp();

  bool Init(const std::string& cluster_name, const std::string& topic_name);
  void Uninit();
  bool Produce(const char* data, size_t data_len, const std::string& key);

 private:
  static void MsgDeliveredCallback(rd_kafka_t* rk,
                                   const rd_kafka_message_t* rkmessage,
                                   void* opaque);
  static int32_t PartitionHashFunc(const rd_kafka_topic_t* rkt,
                                   const void* keydata, size_t keylen,
                                   int32_t partition_cnt, void* rkt_opaque,
                                   void* msg_opaque);

  bool InitRdKafkaConfig();
  bool InitRdKafkaHandle(const std::string& topic_name);

  bool InternalProduce(const char* data, size_t data_len,
                       const std::string& key, void* opaque);

 private:
  rd_kafka_conf_t* m_rd_kafka_conf;
  rd_kafka_topic_conf_t* m_rd_kafka_topic_conf;
  rd_kafka_topic_t* m_rd_kafka_topic;
  rd_kafka_t* m_rd_kafka_handle;
  rd_kafka_resp_err_t m_sync_send_err;

  std::string m_broker_list;

  bool m_is_sync_send;
  bool m_is_init;
  bool is_record_msg_for_send_failed_;
  bool is_speedup_terminate_;
  bool m_fast_exit;
};

#endif
