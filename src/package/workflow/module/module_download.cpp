/*
* Copyright 2019 <Copyright hobot>
* @brief Download module to download module from image server
* @author mengmeng.zhi
* @date 25/Dec/2018
*/
#include "json/json.h"
#include "log/log_module.h"
#include "workflow/module/module_download.h"
#include "package/conf/conf_manager.h"
#include "workflow/worker.hpp"
#include "com_xservice.h"

using namespace XService;

DownloadModule::DownloadModule(
    std::shared_ptr<boost::asio::io_service>& ioService) {
  io_service_ = ioService;
  module_name_ = "DownloadModule";
  module_type_ = kDownloadImage;
}

DownloadModule::~DownloadModule() {}

bool DownloadModule::Initialize() {
  std::string ip_port;
  int ret = CONF_PARSER()->getValue(kS3ProxySection, kIPPort, ip_port);
  if (0 != ret) {
    LOG_ERROR("[DownloadModule::Initialize] ip&port is null");
    return false;
  }
  if (!trans_.Init(io_service_, ip_port)) {
    return false;
  }

  return true;
}

// TODO: batch download for make request
