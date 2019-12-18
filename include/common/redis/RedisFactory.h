/***************************************************
 *  描述:redis工厂类
 *  日期:2019-01-26
 *  作者:jun.chen@horizon.ai
 *  说明:生成RedisClinet和RedisSubscriber
 ****************************************************/
#ifndef __REDIS_FACTORY_H__
#define __REDIS_FACTORY_H__
#include <atomic>
#include "RedisClient.h"
#include "RedisSubscriber.h"
#include "log_module.h"

class CRedisFactory {
 public:
  ~CRedisFactory() {}

  static CRedisFactory* getInstance() {
    static CRedisFactory m_instance;
    return &m_instance;
  }

 private:
  CRedisFactory() {}
  CRedisFactory(const CRedisFactory& other) {}
  std::atomic_int m_io_num{0};
  std::mutex m_mutex;
  bool Init(int io_num) {
    std::lock_guard<std::mutex> lck(m_mutex);
#ifdef _WIN32
    //! Windows netword DLL init
    WORD version = MAKEWORD(2, 2);
    WSADATA data;
    if (WSAStartup(version, &data) != 0) {
      std::cerr << "WSAStartup() failure" << std::endl;
      return false;
    }
#endif
    //一次Init时，设置传入的io个数，后续再创建client，io个数+1
    if (m_io_num == 0) {
      m_io_num = io_num;
    }
    m_io_num++;
    if (m_io_num < 2) m_io_num = 2;
    try {
      //! High availability requires at least 2 io service workers
      cpp_redis::network::set_default_nb_workers(m_io_num);
    }
    catch (std::exception& ex) {
      LOG_ERROR("[CRedisClient::Init] exception fail!{}", ex.what());
      return false;
    }

    return true;
  }

 public:
  std::shared_ptr<CRedisSubscriber> CreateRedisSubscriber(
      QRedisParam param, const std::string& sub_key, sub_callback sub_func) {
    if (!Init(param.io_num)) return nullptr;
    std::shared_ptr<CRedisSubscriber> redis_ptr(
        new CRedisSubscriber(param, sub_key, sub_func));
    if (!redis_ptr->Init()) return nullptr;
    return redis_ptr;
  }
  std::shared_ptr<CRedisClient> CreateRedisClient(QRedisParam param) {
    if (!Init(param.io_num)) return nullptr;
    std::shared_ptr<CRedisClient> redis_ptr(new CRedisClient(param));
    if (!redis_ptr->Init()) return nullptr;
    return redis_ptr;
  }
};

#endif
