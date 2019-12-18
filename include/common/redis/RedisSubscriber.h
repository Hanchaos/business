/***************************************************
 *  描述:redis订阅对象类
 *  日期:2019-01-26
 *  作者:jun.chen@horizon.ai
 *  说明:基于cpp_redis哨兵模式订阅redis
 *  (统一由工厂设置线程数，否则会有问题，已控制为不能独立创建)
 ****************************************************/
#ifndef __REDIS_SUBSCRIBER_H__
#define __REDIS_SUBSCRIBER_H__
#include <cpp_redis/cpp_redis>
#include "RedisDefine.h"

struct SubResult {
  std::string oper_;
  std::string key_;
};

using sub_callback = std::function<void(const SubResult&)>;

class CRedisSubscriber {
 private:
  CRedisSubscriber(QRedisParam param, const std::string& sub_key,
                   sub_callback sub_func);

 public:
  friend class CRedisFactory;
  ~CRedisSubscriber();

 public:
  static bool config_set(const std::string& host, std::size_t port,
                         const std::string& param, const std::string& val,
                         const std::string& password = "");

  bool OK();
  cpp_redis::subscriber& get();
  QRedisParam& param();
  std::string conf_param();
  std::string conf_val();

 private:
  bool Init();
  bool InitSubscriber();
  void SubEvent(const std::string& key, sub_callback callback);

 private:
  QRedisParam param_;
  std::string sub_key_;
  sub_callback sub_func_;
  std::string conf_param_{""};
  std::string conf_val_{""};
  bool isOk_;
  cpp_redis::subscriber sub_;
};

#endif
