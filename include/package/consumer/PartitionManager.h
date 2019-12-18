#ifndef __PARTITION_LIST_H__
#define __PARTITION_LIST_H__
#include <mutex>
#include "DataDefine.h"
#include "kafka_consumer_imp.h"

class CPartitionManager {
 public:
  CPartitionManager(std::shared_ptr<KafkaConsumerImp> kafka_consumer,
                    int commit_num = 5);
  ~CPartitionManager();

  void emplace_back(std::shared_ptr<PM_DATA> data);
  void remove_from_head(int num);
  void do_something(PM_STATUS& status);

 private:
  bool process_todo(std::shared_ptr<PM_DATA> data);
  void do_something_detail(std::shared_ptr<PM_DATA> data);

 private:
  std::shared_ptr<KafkaConsumerImp> m_kafka_consumer;
  int m_commit_num;
  std::mutex m_mutex;
  std::list<std::shared_ptr<PM_DATA> > m_list_data;
};

#endif
