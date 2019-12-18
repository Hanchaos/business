/*
* Copyright 2019 <Copyright hobot>
* @brief Session that reflect to service side
* @author mengmeng.zhi
* @date 25/Dec/2018
*/

#include <list>
#include <vector>
#include <string>
#include "xperf.h"
#include "message/session.h"
#include "conf/conf_manager.h"
#include "workflow/module_wrapper.h"
#include "workflow/flow_manager.h"

using namespace XService;

const std::string kCommonSection = "common";
const std::string kHttpRetryTime = "httpretrytime";

Session::Session(std::shared_ptr<BaseHandler> pCall) : PerfTPInfo("XBusiness") {
  pHandler_ = pCall;
  init();
}

Session::~Session() {
  if (!is_finished) {
    is_finished = true;
    timer_->SetStatus(-1, std::to_string(ErrCode::kError));
  }
  Print(session_id_, uid_);
}

void Session::init() {
  uid_ = pHandler_->getMsgID();
  session_id_ = uid_;
  is_finished = false;
  current_download_url_index_ = 0;
  downloadFailed_ = 0;
  url_count_ = 0;
  int ret = 0;
  http_retry_time_ =
      CONF_PARSER()->getIntValue(kCommonSection, kHttpRetryTime, ret);
  if (0 != ret) {
    http_retry_time_ = 3;
  }
  /*Make the process chain, define the process steps
  * A particular RPC call knows what process steps it should have
  */
  pHandler_->makeProcessChain(proc_chain_);
  current_type_ = kMaxProcType;
  timer_->SetUniqId(uid_);
}

void Session::Process() {
  ProcType proc = this->NextProcessor();
  ModuleWrapper* pModule = FLOW_MANAGER()->getModule(proc);
  if (NULL == pModule) {
    this->Final(ErrCode::kProcTerminate);
    return;
  }

  pModule->Process(shared_from_this());

  return;
}

void Session::AddChain(const ProcType type) { proc_chain_.push_front(type); }

ProcType Session::NextProcessor() {
  if (proc_chain_.empty()) {
    return kNoneProcess;
  }
  auto ret = proc_chain_.front();
  proc_chain_.pop_front();
  return ret;
}

void Session::Final(int err) {
  if (is_finished) {
    return;
  }
  if (err == 0) {
    timer_->SetStatus(0);
  } else {
    timer_->SetStatus(-1, std::to_string(err));
    LOG_ERROR("uid:{},session_id:{},session final err:{}", uid_, session_id_,
              std::to_string(err));
  }

  is_finished = true;
  if (pHandler_->KafkaData_->data_ != nullptr)
    pHandler_->KafkaData_->data_->state = NODE_STATE_DONE;
}