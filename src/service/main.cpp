/*
 * Copyright 2019 <Copyright hobot>
 * @brief Main of XService service
 * @author mengmeng.zhi
 * @modify xuehuan.hong
 * @date 25/Dec/2018
 */

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include "log/log_module.h"
#include "server_version.h"
#include "conf/conf_manager.h"
#include "workflow/flow_manager.h"
#include "MsgConsumer.h"
#include "com_xservice.h"
#include "hobotlog/hobotlog.hpp"

using namespace XService;
int main(int argc, char** argv) {
  if (argc == 2) {
    std::string parm(argv[1]);
    if (std::string::npos != parm.find("-v")) {
      std::cout << PICSERVER_VERSION << std::endl << __DATE__ << std::endl
                << __TIME__ << std::endl;
      return 0;
    }
  }

  /*Init Log*/
  SetLogLevel(HOBOT_LOG_INFO);
  hobotlog::ResultCode errcode;
  std::string logname("xbusiness");
  g_logger = hobotlog::LogModule::GetLogger(logname, "../conf/log.conf", true,
                                            errcode);
  if (!g_logger) {
    std::cout << "GetLogger ["
              << "xbusiness.log"
              << "] failed: " << static_cast<int>(errcode) << std::endl;
    return -1;
  }

  /*Init Configure*/
  std::string conf_file = "../conf/xbusiness.conf";
  if (!CONMGR()->init(conf_file)) {
    LOG_ERROR("Failed to init configure.");

    return -1;
  }
  LOG_INFO("Init configure success.");

  /*Init Flow manager, IO service thread will be started here*/
  if (!FLOW_MANAGER()->init()) {
    LOG_ERROR("Failed to init Flow manager.");

    return -1;
  }
  LOG_INFO("Init Flow Manager success.");

  /* Init redis*/

  /*Init the process worker, worker will use a threadpool to process the
   * sessions*/
  int ret = 0;
  auto work_thread_num =
      CONF_PARSER()->getIntValue(kCommonSection, kWorkThreadNum, ret);
  if (0 != ret) {
    work_thread_num = 8;
  }
  auto max_task_size =
      CONF_PARSER()->getIntValue(kCommonSection, kMaxTaskNum, ret);
  if (0 != ret) {
    max_task_size = MAX_TASK_NUMBER;
  }
  if (!WORKER(Session)->Initialize(work_thread_num, max_task_size)) {
    LOG_ERROR("Failed to init worker.");
    return -1;
  }
  WORKER(Session)->Start();
  LOG_TRACE("Start worker success.");

  // MsgConsumer, singleton
  MsgConsumer* pMsgConsumer = MsgConsumer::get_instance();
  if (NULL == pMsgConsumer) {
    LOG_ERROR("Failed to new MsgConsumer.");
    return -1;
  }
  if (!pMsgConsumer->Initialize()) {
    LOG_ERROR("Failed to init MsgConsumer.");
    return -1;
  }

  LOG_TRACE("xbusiness startup successfully!");

  // infinite loop
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (getchar() == 'q') {
      LOG_TRACE("xbusiness normal exit, you've touched KEY-Q");
      break;
    }
  }

  return 0;
}
