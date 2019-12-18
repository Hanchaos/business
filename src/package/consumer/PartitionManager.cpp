#include "PartitionManager.h"
#include <future>
#include "log_module.h"
#include "XBusinessHandler.h"

CPartitionManager::CPartitionManager(
    std::shared_ptr<KafkaConsumerImp> kafka_consumer, int commit_num)
    : m_kafka_consumer(kafka_consumer), m_commit_num(commit_num) {
  if (commit_num <= 0) m_commit_num = 1;
}

CPartitionManager::~CPartitionManager() {}

void CPartitionManager::emplace_back(std::shared_ptr<PM_DATA> data) {
  std::lock_guard<std::mutex> lck(m_mutex);
  m_list_data.emplace_back(data);
  process_todo(data);
}

void CPartitionManager::remove_from_head(int num) {
  std::lock_guard<std::mutex> lck(m_mutex);
  int i = 0;
  for (auto it = m_list_data.begin(); it != m_list_data.end() && i < num;) {
    m_list_data.erase(it++);
    i++;
  }
}

void CPartitionManager::do_something(PM_STATUS& status) {
  int first_not_done_pos = 0;
  bool force_commit = false;
  int process_num = 0;
  RdKafka::Message* kmsg = NULL;
  {
    std::lock_guard<std::mutex> lck(m_mutex);
    status.cache_size = m_list_data.size();
    bool is_first = true;
    int pos = 0;
    for (auto it = m_list_data.begin(); it != m_list_data.end(); it++) {
      //找到第一个未完成的节点
      if (is_first) {
        if ((*it)->state != NODE_STATE_DONE) {
          is_first = false;
          if (it != m_list_data.begin()) {
            first_not_done_pos = pos;
            auto itTmp = it;
            itTmp--;
            kmsg = (*itTmp)->msg.get();
          }
        } else if (pos == m_list_data.size() - 1) {
          //已经到最后一个元素则强制提交
          is_first = false;
          first_not_done_pos = pos + 1;
          kmsg = (*it)->msg.get();
          force_commit = true;
        }
      }
      //处理todo节点
      if (process_todo(*it)) process_num++;

      //状态统计
      if ((*it)->state == NODE_STATE_TODO) {
        status.todo_num++;
      } else if ((*it)->state == NODE_STATE_DOING) {
        status.doing_num++;
      } else if ((*it)->state == NODE_STATE_DONE) {
        status.done_num++;
      }
      pos++;
    }
  }

  if (force_commit || first_not_done_pos >= m_commit_num) {
    m_kafka_consumer->commit(kmsg);
    remove_from_head(first_not_done_pos);
    status.commit_num = first_not_done_pos;
  }
  status.process_num = process_num;
}

bool CPartitionManager::process_todo(std::shared_ptr<PM_DATA> data) {
  if (data->state == NODE_STATE_TODO) {
    data->state = NODE_STATE_DOING;
    data->stime = time(NULL);
    // auto handler=std::make_shared<CXBusinessHandler>();
    // bool ret=handler->handle(data);
    bool ret = CXBusinessHandler::getInstance()->handle(
        data, (char*)data->msg->payload());
    LOG_INFO("uid[{}] process complated!", data->msg_id);
    return ret;
  }
  return false;
}
