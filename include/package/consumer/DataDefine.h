#ifndef __DATA_DEFINE_H__
#define __DATA_DEFINE_H__
#include <memory>
#include "librdkafka/rdkafkacpp.h"
#include <atomic>

enum {
  NODE_STATE_TODO = 0,
  NODE_STATE_DOING,
  NODE_STATE_DONE,
};

typedef struct {
  std::atomic_uchar state{NODE_STATE_TODO};
  std::atomic_uint stime{0};
  std::string msg_id;
  std::shared_ptr<RdKafka::Message> msg;
} PM_DATA;

typedef struct {
  uint32_t cache_size{0};   //当前总的缓存个数
  float worker_usage{0};    //当前工作线程使用率
  uint32_t process_num{0};  //本次处理多少个todo->doing
  uint32_t commit_num{0};   //本次commit
  uint32_t todo_num{0};     //当前todo节点个数
  uint32_t doing_num{0};    //当前doing节点个数
  uint32_t done_num{0};     //当前done节点个数
} PM_STATUS;
#endif
