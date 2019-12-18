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
#include "workflow/module/module_feature_extract.h"
#include "conf/conf_manager.h"
#include "com_xservice.h"

using namespace XService;

FeatureExtractModule::FeatureExtractModule(
    std::shared_ptr<boost::asio::io_service>& ioService) {
  io_service_ = ioService;
  module_name_ = "FeatureExtractModule";
  module_type_ = kExtractFeature;
}

FeatureExtractModule::~FeatureExtractModule() {}

bool FeatureExtractModule::Initialize() {
  std::string ip_port;
  int ret = CONF_PARSER()->getValue(kDetectSection, kIPPort, ip_port);
  if (0 != ret) {
    LOG_ERROR("[FeatureExtractModule::Initialize] ip&port is null");
    return false;
  }

  if (!trans_.Init(io_service_, ip_port)) {
    return false;
  }

  return true;
}