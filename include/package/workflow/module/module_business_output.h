/*
* Copyright 2019 <Copyright hobot>
* @brief Operate the doc infomation of a business flow event
* @author liangliang.zhao
* @date 18/Mar/2019
*/

#ifndef BUSINESS_OUTPUT_H
#define BUSINESS_OUTPUT_H

#include <map>

#include <mongocxx/client.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/instance.hpp>
#include <workflow/module_factory.h>

namespace XService {

class ModuleFactory;
class Session;

class BusinessSave : public ModuleWrapper {
  FRIEND_FACTORY
 public:
  BusinessSave();

  virtual ~BusinessSave();

  virtual void Process(std::shared_ptr<Session>);

  virtual bool Initialize();

  virtual void Finalize() {}

  int SaveBusinessMongo(std::shared_ptr<Session> session);

  bool SwitchFaceSetIdBySetName(std::shared_ptr<Session> session);

};
}  // namespace XService

#endif  // BUSINESS_OUTPUT_H
