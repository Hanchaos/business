
#include <set>
#include "kafkaProducerImp.h"

#include "kafka_constant.h"
extern "C" {
typedef struct rd_kafka_broker_s rd_kafka_broker_t;
rd_kafka_broker_t *rd_kafka_broker_any(rd_kafka_t *rk, int state,
                                       int (*filter)(rd_kafka_broker_t *rkb,
                                                     void *opaque),
                                       void *opaque);
}
KafkaProducerImp::KafkaProducerImp()
    : m_rd_kafka_conf(NULL),
      m_rd_kafka_topic_conf(NULL),
      m_rd_kafka_topic(NULL),
      m_rd_kafka_handle(NULL),
      m_sync_send_err(RD_KAFKA_RESP_ERR_NO_ERROR),
      m_broker_list(""),
      m_is_init(false),
      m_is_sync_send(false),
      is_record_msg_for_send_failed_(false),
      is_speedup_terminate_(false),
      m_fast_exit(false) {}

KafkaProducerImp::~KafkaProducerImp() {}

bool KafkaProducerImp::Init(const std::string &broker_list,
                            const std::string &topic_name) {

  m_broker_list = broker_list;
  InitRdKafkaConfig();
  InitRdKafkaHandle(topic_name);
  if (m_is_sync_send) {
  }

  return m_is_sync_send;
}

void KafkaProducerImp::Uninit() {
  rd_kafka_poll(m_rd_kafka_handle, 0);
  if (m_is_sync_send) m_is_sync_send = false;

  if (NULL != m_rd_kafka_handle && NULL != m_rd_kafka_topic) {
    int current_poll_time = 0;
    while (rd_kafka_outq_len(m_rd_kafka_handle) > 0) {
      rd_kafka_poll(m_rd_kafka_handle, RD_KAFKA_POLL_TIMIE_OUT_MS);
      if (m_is_sync_send &&
          current_poll_time++ >= RD_KAFKA_SYNC_SEND_UINIT_POLL_TIME) {
        break;
      }
    }

    rd_kafka_topic_destroy(m_rd_kafka_topic);
    m_rd_kafka_topic = NULL;

    rd_kafka_destroy(m_rd_kafka_handle);
    m_rd_kafka_handle = NULL;
  }
}

bool KafkaProducerImp::InternalProduce(const char *data, size_t data_len,
                                       const std::string &key, void *opaque) {
  bool rt = false;

  m_sync_send_err = (rd_kafka_resp_err_t)RD_KAFKA_PRODUCE_ERROR_INIT_VALUE;

  if (NULL == m_rd_kafka_handle ||
      -1 == rd_kafka_produce(
                m_rd_kafka_topic, RD_KAFKA_PARTITION_UA, RD_KAFKA_MSG_F_COPY,
                static_cast<void *>(const_cast<char *>(data)), data_len,
                key.length() > 0 ? key.c_str() : NULL, key.length(), opaque)) {

  } else if (m_is_sync_send) {

  } else {
    rd_kafka_poll(m_rd_kafka_handle, 0);
    rt = true;
  }

  return rt;
}

bool KafkaProducerImp::Produce(const char *data, size_t data_len,
                               const std::string &key) {
  bool rt = false;

  if (m_is_sync_send) {
    rt = InternalProduce(data, data_len, key, 0);
  } else {
    rt = InternalProduce(data, data_len, key, 0);
  }
  return rt;
}

bool KafkaProducerImp::InitRdKafkaHandle(const std::string &topic_name) {
  bool rt = false;

  char err_str[512] = {0};
  m_rd_kafka_handle = rd_kafka_new(RD_KAFKA_PRODUCER, m_rd_kafka_conf, err_str,
                                   sizeof(err_str));
  if (NULL == m_rd_kafka_handle) {
  } else if (0 ==
             rd_kafka_brokers_add(m_rd_kafka_handle, m_broker_list.c_str())) {
  } else {
    m_rd_kafka_topic = rd_kafka_topic_new(m_rd_kafka_handle, topic_name.c_str(),
                                          m_rd_kafka_topic_conf);
    if (NULL == m_rd_kafka_topic) {
    } else {
      rt = true;
    }
  }
  return rt;
}

void KafkaProducerImp::MsgDeliveredCallback(rd_kafka_t *rk,
                                            const rd_kafka_message_t *rkmessage,
                                            void *opaque) {
  KafkaProducerImp *producer = static_cast<KafkaProducerImp *>(opaque);
  if (NULL == producer) {
    return;
  }

  if (producer->m_is_sync_send) {
    producer->m_sync_send_err = rkmessage->err;
  }
  /*else if (rkmessage->err && producer->m_is_sync_send &&
  rd_kafka_broker_any(rk, 4, NULL, NULL))
  {
          if (NULL == producer->m_rd_kafka_handle || -1 ==
  rd_kafka_produce(producer->m_rd_kafka_topic,
                          RD_KAFKA_PARTITION_UA,
                          RD_KAFKA_MSG_F_COPY,
                          rkmessage->payload,
                          rkmessage->len,
                          NULL,
                          0,
                          0))
          {


          }
  }*/

  if (rkmessage->err) {

    if (producer->is_record_msg_for_send_failed_) {
    }
  } else {
  }
}

int32_t KafkaProducerImp::PartitionHashFunc(const rd_kafka_topic_t *rkt,
                                            const void *keydata, size_t keylen,
                                            int32_t partition_cnt,
                                            void *rkt_opaque,
                                            void *msg_opaque) {
  int32_t hit_partition = 0;

  if (keylen > 0 && NULL != keydata) {
    const char *key = static_cast<const char *>(keydata);

    unsigned int hash = 5381;
    for (size_t i = 0; i < keylen; i++) {
      hash = ((hash << 5) + hash) + key[i];
    }

    hit_partition = hash % partition_cnt;

    if (1 != rd_kafka_topic_partition_available(rkt, hit_partition)) {
      hit_partition = 0;
    }
  } else {
    std::set<int32_t> partition_set;
    for (int32_t i = 0; i < partition_cnt; ++i) {
      partition_set.insert(i);
    }

    while (true) {
      hit_partition = rd_kafka_msg_partitioner_random(
          rkt, keydata, keylen, partition_cnt, rkt_opaque, msg_opaque);
      if (1 == rd_kafka_topic_partition_available(rkt, hit_partition)) {
        break;
      } else {
        partition_set.erase(hit_partition);
        if (partition_set.empty()) {
          hit_partition = rd_kafka_msg_partitioner_random(
              rkt, keydata, keylen, partition_cnt, rkt_opaque, msg_opaque);
          ;
          break;
        }
      }
    }
  }

  return hit_partition;
}

bool KafkaProducerImp::InitRdKafkaConfig() {

  bool rt = false;

  m_rd_kafka_conf = rd_kafka_conf_new();
  if (NULL != m_rd_kafka_conf) {
    rd_kafka_conf_set_opaque(m_rd_kafka_conf, static_cast<void *>(this));

    m_rd_kafka_topic_conf = rd_kafka_topic_conf_new();
    if (NULL == m_rd_kafka_topic_conf) {

    } else {
      rd_kafka_conf_set_dr_msg_cb(m_rd_kafka_conf,
                                  &KafkaProducerImp::MsgDeliveredCallback);

      rd_kafka_topic_conf_set_partitioner_cb(
          m_rd_kafka_topic_conf, &KafkaProducerImp::PartitionHashFunc);
      rt = true;
    }
  } else {
  }

  return rt;
}