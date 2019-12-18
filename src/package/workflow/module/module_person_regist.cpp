/*
* Copyright 2019 <Copyright hobot>
* @brief Person register module, register person to XID
* @author mengmeng.zhi
* @date 25/Dec/2018
*/

#include <thread>
#include <chrono>
#include <iostream>
#include "common/com_func.h"
#include "log/log_module.h"
#include "workflow/module/module_person_regist.h"
#include "conf/conf_manager.h"
#include "com_xservice.h"

using namespace XService;

// Register request is sent to XID too
static const std::string kSearchSection = "search";
static const std::string kIPPort = "ipport";

PersonRegisterModule::PersonRegisterModule(
    std::shared_ptr<boost::asio::io_service>& ioService) {
  io_service_ = ioService;
  module_name_ = "PersonRegisterModule";
  module_type_ = kRegisterPerson;
}

PersonRegisterModule::~PersonRegisterModule() {}

bool PersonRegisterModule::Initialize() {
  std::string ip_port;
  int ret = CONF_PARSER()->getValue(kSearchSection, kIPPort, ip_port);
  if (0 != ret) {
    LOG_ERROR("[PersonRegisterModule::Initialize] ip&port is null");
    return false;
  }

  if (!trans_.Init(io_service_, ip_port)) {
    return false;
  }

  return true;
}

void PersonRegisterModule::Process(std::shared_ptr<Session> session) {
  session->DoRecord("PersonRegisterModule::Process");
  session->timer_->SetBigTimer(module_name_);
  LOG_INFO("[PersonRegisterModule] aotoregister:{}", session->is_autoregister_);

  if (!session->is_autoregister_ ||
      (session->xfr_result.can_autoreg != XService::kCanRegist)) {
    // If all the processor has been done, the session will be finished.
    if (!session->HasNextProcessor()) {
      session->Final(ErrCode::kSuccess);
      return;
    }
    WORKER(Session)->PushMsg(session);
    return;
  } else {
    LOG_INFO(
        "[PersonRegistModule::ProcResponse]uid:{} session_id:{} start to "
        "register",
        session->uid_, session->session_id_);
    RequestInfo request;
    if (!MakeRequest(session, request)) {
      session->Final(ErrCode::kProcTerminate);
      return;
    }
    SendRequest(session, request);
  }
}
