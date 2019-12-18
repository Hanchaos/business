/*
* Copyright 2019 <Copyright hobot>
* @brief Download module to download module from image server
* @author mengmeng.zhi
* @date 25/Dec/2018
*/
#ifndef FEATURE_SEARCH_STATIC_MODULE_H
#define FEATURE_SEARCH_STATIC_MODULE_H

#include "workflow/http_module_wrapper.h"
#include <workflow/module_factory.h>

namespace XService {

class TraceSpaceModule : public HTTPModuleWrapper {
  FRIEND_FACTORY
 protected:
  explicit TraceSpaceModule(
      std::shared_ptr<boost::asio::io_service>& ioService);

  ~TraceSpaceModule();

  virtual bool Initialize();
};
};
#endif  // FEATURE_SEARCH_STATIC_MODULE_H
