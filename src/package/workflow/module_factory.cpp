/*
* Copyright 2019 <Copyright hobot>
* @brief All the modules shold be created from the factory
* in different steps
* @author mengmeng.zhi
* @date 25/Dec/2018
*/

#include "log/log_module.h"
#include <workflow/module_factory.h>
#include <workflow/module/module_download.h>
#include <workflow/module/module_kafka_output.h>
#include <workflow/module/module_picture_waiting.h>
#include <workflow/module/module_business_output.h>
#include <workflow/module/module_trace_space.h>
#include <workflow/module/module_generate_url.h>
#include <workflow/module/module_get_plugin.h>

using namespace XService;

ModuleWrapper* ModuleFactory::createModule(
    ProcType type, std::shared_ptr<boost::asio::io_service> ioService) {
  switch (type) {
    /*case kPreProcess:
        return createPreProcessModule();
    */
    case kDownloadImage:
      return createDownloadModule(ioService);

    case kOutputKafka:
      return createOutPutKafkaModule();

    case kPicWait:
      return createPicWaitModule();

    case kBusinessSave:
      return createBusinessOutputModule();

    case kTraceSpace:
      return createTraceSpaceModule(ioService);

    case kGenerateUrl:
      return createGenerateUrlModule(ioService);

    case kGetPlugin:
      return createGetPluginModule();

    default:
      LOG_ERROR("[ModuleFactory::createModule] Unknown module type {}", type);
      return NULL;
  }
}

ModuleWrapper* ModuleFactory::createPreProcessModule() {
  /*Preprocess* pModule = new (std::nothrow) Preprocess();
  return std::shared_ptr<ModuleWrapper>(pModule);*/
  return NULL;
}

ModuleWrapper* ModuleFactory::createDownloadModule(
    std::shared_ptr<boost::asio::io_service>& ioService) {
  DownloadModule* pModule = new (std::nothrow) DownloadModule(ioService);
  return pModule;
}

ModuleWrapper* ModuleFactory::createOutPutKafkaModule() {
  OutPutKafkaModule* pModule = new (std::nothrow) OutPutKafkaModule();
  return pModule;
}

ModuleWrapper* ModuleFactory::createPicWaitModule() {
  PictureWaitModule* pModule = new (std::nothrow) PictureWaitModule();
  return pModule;
}

ModuleWrapper* ModuleFactory::createBusinessOutputModule() {
  BusinessSave* pModule = new (std::nothrow) BusinessSave();
  return pModule;
}

ModuleWrapper* ModuleFactory::createTraceSpaceModule(
    std::shared_ptr<boost::asio::io_service>& ioService){
  TraceSpaceModule* pModule = new (std::nothrow) TraceSpaceModule(ioService);
  return pModule;
}

ModuleWrapper* ModuleFactory::createGenerateUrlModule(
    std::shared_ptr<boost::asio::io_service>& ioService){
  GenerateUrlModule* pModule = new (std::nothrow) GenerateUrlModule(ioService);
  return pModule;
}

ModuleWrapper* ModuleFactory::createGetPluginModule() {
  GetPluginModule* pModule = new (std::nothrow) GetPluginModule();
  return pModule;
}

ModuleWrapper* ModuleFactory::createFinalModule() {
  /*FinalProcessor* pModule = new (std::nothrow) FinalProcessor();
  return std::shared_ptr<ModuleWrapper>(pModule);*/
  return NULL;
}

