/*
* Copyright 2019 <Copyright hobot>
* @brief Download module to download module from image server
* @author mengmeng.zhi
* @date 25/Dec/2018
*/

#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#include <string>
#include "workflow/http_module_wrapper.h"
#include <workflow/module_factory.h>
#include <transponder/transponder.h>

namespace XService {
class DownloadModule : public HTTPModuleWrapper {
  FRIEND_FACTORY
 protected:
  explicit DownloadModule(std::shared_ptr<boost::asio::io_service>& ioService);

  virtual ~DownloadModule();

  virtual bool Initialize();
};
};
#endif  // DOWNLOAD_H
