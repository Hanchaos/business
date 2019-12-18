#ifndef KAFKA_CONSUMER_IMP_H
#define KAFKA_CONSUMER_IMP_H
#include "librdkafka/rdkafkacpp.h"
#include "kafka_consumer_imp_cb.h"
#include <memory>
#include <thread>
#include <atomic>

enum {
  CONSUEMR_STATE_RUN = 0,
  CONSUEMR_STATE_PAUSE
};

using FuncCallBack = std::function<void(std::shared_ptr<void>)>;

class KafkaConsumerImp {
 public:
  /*
    brokers:ip:port,ip:port
    callback:工作线程的处理回调
    is_auto_commit:是否手动提交
    cheyuexian.che
  */
  explicit KafkaConsumerImp(const std::string& brokers,
                            FuncCallBack callback = nullptr,
                            int is_auto_commit = 0);
  ~KafkaConsumerImp();

 public:
  bool init();
  void release();
  bool subscribe(const std::string& group,
                 const std::vector<std::string>& topics);
  bool resume();
  bool pause();
  void startup();
  void shutdown();
  void commit();
  void commit(RdKafka::Message* msg);
  void waitForThreadExit();

 private:
  void threadFun();

 private:
  std::string m_broker_list;
  FuncCallBack m_callback;
  int m_is_auto_commit;
  // enable kafka conf dump, default: false
  bool m_enable_conf_dump;

  std::atomic_bool m_run;
  std::atomic_bool m_cur_state{CONSUEMR_STATE_RUN};
  std::shared_ptr<std::thread> m_thread;

  std::shared_ptr<RdKafka::Conf> m_conf;
  std::shared_ptr<RdKafka::KafkaConsumer> m_consumer;
  ExampleRebalanceCb m_rebalance_cb;
  ExampleEventCb m_event_cb;
};

#endif
