/*
* Copyright 2019 <Copyright hobot>
* @brief Operate the doc infomation of a business flow event
* @author liangliang.zhao
* @date 18/Mar/2019
*/

#ifndef MONGO_CLIENT_POOL_H
#define MONGO_CLIENT_POOL_H

#include <map>

#include <mongocxx/client.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/instance.hpp>
#include "conf/conf_manager.h"


namespace XService {

class MongoClientPool {
  public:

  static MongoClientPool* getInstance() {
    static MongoClientPool m_instance;
    return &m_instance;
  }

  bool Initialize();

 std::unique_ptr<mongocxx::pool> pool_ = nullptr;

 private:
  static mongocxx::instance inst_;
  //std::unique_ptr<mongocxx::pool> pool_ = nullptr;
  std::string mongo_url_;
};
}  // namespace XService

#endif  // MONGO_CLIENT_POOL_H
