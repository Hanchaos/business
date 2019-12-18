/*
* Copyright 2019 <Copyright hobot>
* @brief Operate the doc infomation of a business flow event
* @author liangliang.zhao
* @date 18/Mar/2019
*/

#ifndef GET_PLUGIN_H
#define GET_PLUGIN_H

#include <map>

#include <mongocxx/client.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/instance.hpp>
#include <workflow/module_factory.h>

namespace XService {

class ModuleFactory;
class Session;

class GetPluginModule : public ModuleWrapper {
  FRIEND_FACTORY
 public:
  GetPluginModule();

  virtual ~GetPluginModule();

  virtual void Process(std::shared_ptr<Session>);


  void repairProcChain(std::shared_ptr<Session> session);

  bool getInfoByAppId(std::shared_ptr<Session> session);

  virtual bool Initialize();

  virtual void Finalize() {}

 private:
  std::string mongo_dbname_;
};
}  // namespace XService

#endif  // GET_PLUGIN_H