/*
* Copyright 2019 <Copyright hobot>
* @brief Download module to download module from image server
* @author mengmeng.zhi
* @date 25/Dec/2018
*/

#include <thread>
#include <chrono>
#include <iostream>
#include "common/com_func.h"
#include "log/log_module.h"
#include "workflow/module/module_feature_search_static.h"
#include "conf/conf_manager.h"

using namespace XService;

static const std::string kSearchSection = "search";
static const std::string kIPPort = "ipport";

FeatureSearchStaticModule::FeatureSearchStaticModule(
    std::shared_ptr<boost::asio::io_service>& ioService) {
  io_service_ = ioService;
  module_name_ = "FeatureSearchStaticModule";
  module_type_ = kSearchFeatureStatic;
}

FeatureSearchStaticModule::~FeatureSearchStaticModule() {}

bool FeatureSearchStaticModule::Initialize() {
  std::string ip_port;
  int ret = CONF_PARSER()->getValue(kSearchSection, kIPPort, ip_port);
  if (0 != ret) {
    LOG_ERROR("[FeatureSearchModule::Initialize] ip&port is null");
    return false;
  }

  if (!trans_.Init(io_service_, ip_port)) {
    return false;
  }

  return true;
}
