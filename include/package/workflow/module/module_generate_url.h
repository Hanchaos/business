/*
* Copyright 2019 <Copyright hobot>
* @brief Download module to download module from image server
* @author mengmeng.zhi
* @date 25/Dec/2018
*/
#ifndef FEATURE_GENERATE_URL_H
#define FEATURE_GENERATE_URL_H

#include "workflow/http_module_wrapper.h"
#include <workflow/module_factory.h>
#include <string>

namespace XService {

class GenerateUrlModule : public HTTPModuleWrapper {
  FRIEND_FACTORY
 protected:
  explicit GenerateUrlModule(
      std::shared_ptr<boost::asio::io_service>& ioService);

  ~GenerateUrlModule();

  virtual bool Initialize();

};
};
#endif  // FEATURE_SEARCH_H
