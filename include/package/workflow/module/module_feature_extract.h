/*
* Copyright 2019 <Copyright hobot>
* @brief Download module to download module from image server
* @author mengmeng.zhi
* @date 25/Dec/2018
*/
#ifndef FEATURE_EXTRACT_H
#define FEATURE_EXTRACT_H

#include "workflow/http_module_wrapper.h"
#include <workflow/module_factory.h>

namespace XService {

class FeatureExtractModule : public HTTPModuleWrapper {
  FRIEND_FACTORY
 protected:
  explicit FeatureExtractModule(
      std::shared_ptr<boost::asio::io_service>& ioService);

  ~FeatureExtractModule();

  virtual bool Initialize();
};
};
#endif  // FEATURE_EXTRACT_H
