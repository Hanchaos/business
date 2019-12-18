/***************************************************
 *  描述:获取任务信息类
 *  日期:2019-01-28
 *  作者:jun.chen@horizon.ai
 *  说明:基于LRU+redis获取任务
 *       任务更新采用事件订阅结合定期刷新
 ****************************************************/
#ifndef __MODULE_PICTURE_WAITING_H__
#define __MODULE_PICTURE_WAITING_H__
#include "RedisFactory.h"
#include "lrucache.h"
#include "timer.h"
#include "workflow/module_wrapper.h"
namespace XService {
class PictureWaitModule : public ModuleWrapper {
 public:
  PictureWaitModule();
  enum PictureWaitResult {
    PASS = 0,  //数据继续后续流程
    WAIT = 1   //数据已经缓存到redis中等到图片上传触发后续流程本次流程结束
  };

 public:
  ~PictureWaitModule();
  bool Initialize();
  bool Init(const QRedisParam& param);

  void Process(std::shared_ptr<Session>);

  int getResultOfWait(std::shared_ptr<Session>);

  void Finalize() {}

  int handleWithoutKey(std::shared_ptr<Session> &session, std::string &key, std::string &payload, int &ptype);

  int handleWithKey(std::shared_ptr<Session> &session, std::string &key, std::string &vaule, std::string &payload, int &ptype);


 private:
  const size_t redlock_acquire_timeout_ = 5000;  //分布式锁获取超时时间ms
  const size_t redlock_expire_timeout_ = 5000;  //分布式锁获取超时时间ms
  const size_t picturewait_cache_timeout =
      10 * 60 * 1000;  //分布式锁获取超时时间ms

  std::shared_ptr<CRedisClient> rediscli_;
};
}
#endif
