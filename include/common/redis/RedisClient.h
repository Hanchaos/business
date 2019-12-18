/***************************************************
 *  描述:redis读写对象类
 *  日期:2019-01-26
 *  作者:jun.chen@horizon.ai
 *  说明:基于cpp_redis哨兵模式读写redis
 *  (统一由工厂设置线程数，否则会有问题，已控制为不能独立创建)
 ****************************************************/
#ifndef __REDIS_CLIENT_H__
#define __REDIS_CLIENT_H__
#include <cpp_redis/cpp_redis>
#include <string>
#include "RedisDefine.h"

class CRedisClient {
 private:
  explicit CRedisClient(QRedisParam param);

 public:
  friend class CRedisFactory;
  ~CRedisClient();

  bool GetValueByKey(const std::string& key, std::string& data);

  // nx:false means force-set, otherwise means setnx
  bool SetAdvancedKeyValue(const std::string& key, const std::string& data,
                           int timeout_ms, bool nx = false);

  bool SetKeyValue(const std::string& key, const std::string& data);

  bool ExpireKey(const std::string& key, int timeout);

  bool RedLock(const std::string& key, const std::string& data,
               int64_t acquiretimeout, long expiretimeout);

  bool RedUnlock(const std::string& key, const std::string& data);

  bool OK();
  cpp_redis::client& get();
  QRedisParam& param();

 private:
  bool Init();
  bool InitClient();
  const size_t sync_wait_timeout_ = 5000;  // ms
  const size_t lock_check_period_ = 10;  //ms

 private:
  QRedisParam param_;
  bool isOk_;
  cpp_redis::client client_;
};

#endif
