/*
* Copyright 2019 <Copyright hobot>
* @brief Operate the doc infomation of a person
* @author mengmeng.zhi
* @date 25/Dec/2018
*/
#include <mongocxx/client.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/exception/exception.hpp>

#include "common/com_xservice.h"
#include "common/mongo/mongo_client_pool.h"
#include "conf/conf_manager.h"
#include "log/log_module.h"


using namespace XService;

static mongocxx::instance inst_{};

bool MongoClientPool::Initialize() {
  int ret = CONF_PARSER()->getValue(kMongoBusinessSection, kMongoUrl, mongo_url_);
  if (0 != ret) {
    LOG_ERROR("[BusinessSaveModule::Initialize] mongo url is null");
    return false;
  }
  
  try {
    pool_ = std::move(std::unique_ptr<mongocxx::pool>(
        new mongocxx::pool(mongocxx::uri(mongo_url_))));
    if (!pool_) {
      LOG_INFO("[MongoClientPool::Initialize] fail to get a mongo pool of {}",
               mongo_url_);
      return false;
    }
  }
  catch (mongocxx::exception &e) {
    LOG_ERROR("[MongoClientPool::Initialize] fail to get a mongo pool of {}",
              mongo_url_);
    return false;
  }
  return true;

}


