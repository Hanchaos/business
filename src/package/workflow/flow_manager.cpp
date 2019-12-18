/*
* Copyright 2019 <Copyright hobot>
* @brief Manage the work flow
* in different steps
* @author mengmeng.zhi
* @date 25/Dec/2018
*/

#include <package/workflow/flow_manager.h>
#include <workflow/module_wrapper.h>
#include "workflow/module_factory.h"
#include <conf/conf_manager.h>
#include "common/mongo/mongo_client_pool.h"
#include "com_xservice.h"

using namespace XService;

FlowManager* FlowManager::pInstance_ = NULL;
std::mutex FlowManager::singl_mutex_;

const std::string kCommonSection = "common";
const std::string kIOThreadNum = "io_thread_num";

FlowManager* FlowManager::getInstance() {
  if (NULL == pInstance_) {
    std::lock_guard<std::mutex> lck(singl_mutex_);
    if (NULL == pInstance_) {
      pInstance_ = new FlowManager();
    }
  }

  return pInstance_;
}

bool FlowManager::init() {
  // Before init modules, init the io_service first
  // io_service is used for send and receive http message
  io_service_ = std::make_shared<boost::asio::io_service>();

  /*Create al module that needed for the process steps*/
  for (ProcType type = kDownloadImage; type < kMaxProcType;
       type = (ProcType)(type + 1)) {
    ModuleWrapper* module = ModuleFactory::createModule(type, io_service_);
    if (!module) {
      LOG_ERROR("[FlowManager::init] Faied to create module {}", type);
      return false;
    }

    if (!module->Initialize()) {
      LOG_ERROR("[FlowManager::init] Faied to init module {}", type);
      return false;
    }

    if (module_dict_.end() != module_dict_.find(type)) {
      continue;
    }
    module_dict_[type] = module;
  }

  if (!MongoClientPool::getInstance()->Initialize()) {
    return false;
  }

  // Start IO thread after all the modules been created, module will send
  // request
  // and get response from io_service
  dummy_work_ =
      std::make_shared<boost::asio::io_service::work>(*io_service_.get());
  int ret = 0;
  auto io_thread_num =
      CONF_PARSER()->getIntValue(kCommonSection, kIOThreadNum, ret);
  if (0 != ret) {
    io_thread_num = 8;
  }

  for (int i = 0; i < io_thread_num; ++i) {
    io_workers_.emplace_back([this] { io_service_->run(); });
  }

  return true;
}

void FlowManager::finalize() {
  // After reset the dummy, if there is no more work on the io service,
  // the io thread will be finished.
  dummy_work_.reset();
  for (auto& worker : io_workers_) {
    worker.join();
  }
}

ModuleWrapper* FlowManager::getModule(ProcType procType) {
  if (module_dict_.end() == module_dict_.find(procType)) {
    return NULL;
  }

  return module_dict_[procType];
}
