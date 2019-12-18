/***************************************************
 *  描述:获取任务信息类
 *  日期:2019-01-28
 *  作者:jun.chen@horizon.ai
 *  说明:基于LRU+redis获取任务
 *       任务更新采用事件订阅结合定期刷新
 ****************************************************/
#ifndef __MODULE_TASK_HANDLE_H__
#define __MODULE_TASK_HANDLE_H__
#include "RedisFactory.h"
#include "lrucache.h"
#include "timer.h"
#include "workflow/module_wrapper.h"
namespace XService {
class TaskHandleModule : public ModuleWrapper {
 public:
  TaskHandleModule();

 public:
  ~TaskHandleModule();
  bool Initialize();
  bool Init(const QRedisParam& param, int refresh_interval);
  bool GetTask(const std::string& key, std::string& data);
  bool getAndParseTask(std::shared_ptr<Session> session);
  void Process(std::shared_ptr<Session>);

  void Finalize() {}

 private:
  void RedisCB(const SubResult& res);
  int RefreshTaskCache();

 private:
  const size_t cache_max_size_ = 3000;
  const size_t cache_elasticity_ = 0;
  // timeout, milliseconds
  const size_t sync_wait_timeout_ = 5000;
  lrucache::Cache<std::string, std::string, std::mutex>* taskcache_;
  std::shared_ptr<CRedisClient> rediscli_;
  std::shared_ptr<CRedisSubscriber> redissub_;
  Timer* t_;
};
}
#endif
