/*
* Copyright 2019 <Copyright hobot>
* @brief Download module to download module from image server
* @author mengmeng.zhi
* @date 25/Dec/2018
*/
#ifndef FEATURE_SEARCH_H
#define FEATURE_SEARCH_H

#include "workflow/http_module_wrapper.h"
#include <workflow/module_factory.h>
#include <string>

namespace XService {

class FeatureSearchModule : public HTTPModuleWrapper {
  FRIEND_FACTORY
 protected:
  explicit FeatureSearchModule(
      std::shared_ptr<boost::asio::io_service>& ioService);

  ~FeatureSearchModule();

  virtual bool Initialize();

  virtual void Process(std::shared_ptr<Session>);

  bool isAutoReg(std::string set_name);
};
};
#endif  // FEATURE_SEARCH_H
