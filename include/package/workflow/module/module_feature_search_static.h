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

class FeatureSearchStaticModule : public HTTPModuleWrapper {
  FRIEND_FACTORY
 protected:
  explicit FeatureSearchStaticModule(
      std::shared_ptr<boost::asio::io_service>& ioService);

  ~FeatureSearchStaticModule();

  virtual bool Initialize();
};
};
#endif  // FEATURE_SEARCH_STATIC_MODULE_H
