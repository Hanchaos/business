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
#include "workflow/module/module_trace_space.h"
#include "conf/conf_manager.h"

using namespace XService;

static const std::string kTraceSpaceSection = "trace_space";
static const std::string kIPPort = "ipport";

TraceSpaceModule::TraceSpaceModule(
    std::shared_ptr<boost::asio::io_service>& ioService) {
  io_service_ = ioService;
  module_name_ = "TraceSpaceModule";
  module_type_ = kTraceSpace;
}

TraceSpaceModule::~TraceSpaceModule() {}

bool TraceSpaceModule::Initialize() {
  std::string ip_port;
  int ret = CONF_PARSER()->getValue(kTraceSpaceSection, kIPPort, ip_port);
  if (0 != ret) {
    LOG_ERROR("[TraceSpaceModule::Initialize] ip&port is null");
    return false;
  }

  if (!trans_.Init(io_service_, ip_port)) {
    return false;
  }

  return true;
}
