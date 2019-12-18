/*
* Copyright 2019 <Copyright hobot>
* @brief Download module to download module from image server
* @author mengmeng.zhi
* @date 25/Dec/2018
*/
#ifndef PERSON_REGISTER_H
#define PERSON_REGISTER_H

#include "workflow/http_module_wrapper.h"
#include <workflow/module_factory.h>

namespace XService {

class PersonRegisterModule : public HTTPModuleWrapper {
  FRIEND_FACTORY
 protected:
  explicit PersonRegisterModule(
      std::shared_ptr<boost::asio::io_service>& ioService);

  ~PersonRegisterModule();

  virtual bool Initialize();

  void Process(std::shared_ptr<Session> session);
};
};
#endif  // PERSON_REGISTER_H
