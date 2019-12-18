/*
* Copyright 2019 <Copyright hobot>
* @brief Manage the work flow
* in different steps
* @author mengmeng.zhi
* @date 25/Dec/2018
*/
#ifndef FLOW_MANAGER_
#define FLOW_MANAGER_

#include <mutex>
#include <map>
#include <vector>
#include <boost/asio.hpp>

#include <workflow/module_wrapper.h>
#include <handler/base_handler.h>

namespace XService {
class FlowManager {
 public:
  /*
  * @brief Init the flow manager
  *   Costruct all the module and http client that need for flows.
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  bool init();

  /*
  * @brief Init the flow manager
  *   Costruct all the module and http client that need for flows.
  * TODO: implement this function in the future.
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  void finalize();

  /*
  * @brief Get the module by proc type
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  ModuleWrapper* getModule(ProcType proc);

  /*
  * @brief Singleton for FlowManager
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  static FlowManager* getInstance();

 private:
  /*Mapping relationship of ProcType and modules*/
  std::map<ProcType, ModuleWrapper*> module_dict_;

  /*All the io worker threads*/
  std::vector<std::thread> io_workers_;
  /*Use boost asio as the io poller*/
  std::shared_ptr<boost::asio::io_service> io_service_;
  std::shared_ptr<boost::asio::io_service::work> dummy_work_;

  static FlowManager* pInstance_;
  static std::mutex singl_mutex_;  // mutex for singleton
};

};  // namespace XService

#endif  // FLOW_MANAGER_

#define FLOW_MANAGER() FlowManager::getInstance()