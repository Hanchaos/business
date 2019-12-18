/*
* Copyright 2019 <Copyright hobot>
* @brief Download module to download module from image server
* @author mengmeng.zhi
* @date 25/Dec/2018
*/

#include <thread>
#include <chrono>
#include <iostream>
#include "common/com_func.h"
#include "log/log_module.h"
#include "workflow/module/module_feature_search.h"
#include "conf/conf_manager.h"

using namespace XService;

static const std::string kSearchSection = "search";
static const std::string kIPPort = "ipport";

FeatureSearchModule::FeatureSearchModule(
    std::shared_ptr<boost::asio::io_service>& ioService) {
  io_service_ = ioService;
  module_name_ = "FeatureSearchModule";
  module_type_ = kSearchFeature;
}

FeatureSearchModule::~FeatureSearchModule() {}

bool FeatureSearchModule::Initialize() {
  std::string ip_port;
  int ret = CONF_PARSER()->getValue(kSearchSection, kIPPort, ip_port);
  if (0 != ret) {
    LOG_ERROR("[FeatureSearchModule::Initialize] ip&port is null");
    return false;
  }

  if (!trans_.Init(io_service_, ip_port)) {
    return false;
  }

  return true;
}

void FeatureSearchModule::Process(std::shared_ptr<Session> session) {
  session->DoRecord("FeatureSearchModule::Process");
  session->timer_->SetBigTimer(module_name_);
  // 判断除了vip库之外，是否还有auto_reg的库需要检索
  bool bHasAutoRegSet = false;

  std::list<std::string> set_list;
  if (session->pHandler_->getAutoRegSet(set_list)) {
    bHasAutoRegSet = true;
  }

  //前面已经检索过一次，如果已经发现匹配了静态库，或者已经没有auto_reg的库要检索，就不进行第二次检索
  if (session->is_static_matched_ || !bHasAutoRegSet) {
    LOG_DEBUG(
        "[FeatureSearchModule::Process] matche static library:{}, have to "
        "seach in auto reg library:{}, no need to search again.",
        session->is_static_matched_ ? "true" : "false",
        bHasAutoRegSet ? "true" : "false");

    // If all the processor has been done, the session will be finished.
    if (!session->HasNextProcessor()) {
      session->Final(ErrCode::kSuccess);
      return;
    }
    WORKER(Session)->PushMsg(session);
    return;
  }

  /*
  LOG_DEBUG("[FeatureSearchModule::Process]uid:{} {} start to MakeRequest",
            session->pHandler_->uid_,
            session->session_id_);
  */
  RequestInfo request;
  if (!MakeRequest(session, request)) {
    session->Final(ErrCode::kProcTerminate);
    return;
  }

  SendRequest(session, request);
  return;
}
