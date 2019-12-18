/*
* Copyright 2019 <Copyright hobot>
* @brief All the modules shold be created from the factory
* in different steps
* @author mengmeng.zhi
* @date 25/Dec/2018
*/
#ifndef MODULE_FACTORY_H
#define MODULE_FACTORY_H

#include <memory>
#include <boost/asio.hpp>

#include <workflow/module_wrapper.h>

namespace XService {
class ModuleFactory {
 public:
  static ModuleWrapper* createModule(
      ProcType type, std::shared_ptr<boost::asio::io_service> ioService);

 private:
  static ModuleWrapper* createPreProcessModule();

  static ModuleWrapper* createDownloadModule(
      std::shared_ptr<boost::asio::io_service>& ioService);

  static ModuleWrapper* createOutPutKafkaModule();

  static ModuleWrapper* createPicWaitModule();

  static ModuleWrapper* createFinalModule();

  static ModuleWrapper* createBusinessOutputModule();

  static ModuleWrapper* createTraceSpaceModule(
      std::shared_ptr<boost::asio::io_service>& ioService);

  static ModuleWrapper* createGenerateUrlModule(
      std::shared_ptr<boost::asio::io_service>& ioService);

  static ModuleWrapper* createGetPluginModule();

};
};

#define FRIEND_FACTORY friend class XService::ModuleFactory;

#endif