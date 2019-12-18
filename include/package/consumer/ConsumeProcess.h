#ifndef __CONSUMER_PROCESS_H__
#define __CONSUMER_PROCESS_H__
#include "DataDefine.h"
#include <memory>
#include <map>
#include <list>
#include <mutex>
#include "PartitionManager.h"
#include "kafka_consumer_imp.h"
#include "ThreadSAP.h"
#include "boost/thread/pthread/shared_mutex.hpp"

typedef struct {
  std::atomic_bool is_run{true};
  std::thread thd;
} THREAD_CONTROLL;

typedef std::map<int32_t, std::shared_ptr<CPartitionManager> >
    CPartitionManagerMap;

class CConsumeProcess : public ThreadSAP {
 public:
  CConsumeProcess();
  virtual ~CConsumeProcess();

  bool Init();
  void Release();
  void process(std::shared_ptr<void> msg);

 private:
  virtual void* Run();
  bool gracefully_exit();

 private:
  // Kafka消费者处理回调函数
  static FuncCallBack m_process_func;
  // Kafka消费者
  std::shared_ptr<KafkaConsumerImp> m_kafka_consumer;
  // Kafka各分区消费数据缓存
  boost::shared_mutex m_partition_managers_mutex;
  CPartitionManagerMap m_partition_managers;
  // Run线程控制
  std::atomic_bool m_is_run;
  // Kafka commit num
  int m_commit_num;
  // Max cache size
  int m_max_cache_size;
};

#endif
