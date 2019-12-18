/*
* Copyright 2019 <Copyright hobot>
* @brief All the modules shold be created from the factory
* in different steps
* @author mengmeng.zhi
* @date 25/Dec/2018
*/

#include <thread>
#include <chrono>
#include "log/log_module.h"
#include "package/workflow/http_module_wrapper.h"
#include "message/session.h"

using namespace XService;

HTTPModuleWrapper::HTTPModuleWrapper() {
  concurrent_requests_ = 0;
  failed_requests_ = 0;
}

void HTTPModuleWrapper::SendRequest(std::shared_ptr<Session> session,
                                    RequestInfo &request) {
  std::istringstream content(request.body_);
  SimpleWeb::CaseInsensitiveMultimap headers;
  for (const auto &couple : request.extra_) {
    headers.insert(couple);
  }
  session->timer_->SetSelfBegin(module_name_ +
                                std::to_string(session->time_track_));
  concurrent_requests_++;
  trans_.Request(request.method_, request.path_, headers, content,
                 [this, session](std::shared_ptr<HttpClient::Response> response,
                                 const SimpleWeb::error_code &ec) {
    this->concurrent_requests_--;
    session->timer_->SetSelfEnd(module_name_ +
                                std::to_string(session->time_track_++));
    if (!ec) {
      this->ProcResponse(session, response);
    } else {
      LOG_WARN("[HTTPModuleWrapper::SendRequest] {} error {}: {}",
               session->session_id_, ec.value(), ec.message());
      failed_requests_++;
      this->HttpErrorProcess(session);
    }
  });
}

void HTTPModuleWrapper::Process(std::shared_ptr<Session> session) {
  session->timer_->SetBigTimer(module_name_);
  RequestInfo request;
  if (!MakeRequest(session, request)) {
    session->Final(ErrCode::kProcTerminate);
    return;
  }
  SendRequest(session, request);
}

void HTTPModuleWrapper::HttpErrorProcess(std::shared_ptr<Session> session) {
  /*if (session->http_retry_time_ > 0) {
    session->http_retry_time_--;*/
    //后续改成并行下载，该判断需优化
  if (module_type_ == XService::kDownloadImage) {
    session->pHandler_->KafkaData_->RetreatUrl();
  }

  if (session->http_retry_time_ > 0) {
    session->http_retry_time_--;
    session->AddChain(module_type_);
    WORKER(Session)->PushMsg(session);
  }
  else {
      LOG_WARN("[HTTPModuleWrapper::SendRequest] do not have more retry time in module: {}",module_name_);
      session->Final(ErrCode::kHttpConnError);
  }

}

bool HTTPModuleWrapper::MakeRequest(std::shared_ptr<Session> session,
                                    RequestInfo &request) {
  bool bRet = true;
  do {
    if (!session->pHandler_.get()) {
      LOG_ERROR("[{}::MakeRequest] RPC call object is null in session.",
                module_name_);
      bRet = false;

      break;
    }

    LOG_TRACE("[{}::MakeRequest] uid:{} session_id:{} Start to make request.",
              module_name_, session->uid_, session->session_id_);

    bRet = session->pHandler_->makeRequest(session, request, module_type_);
    if (!bRet) {
      LOG_ERROR(
          "[{}::MakeRequest] uid:{} session_id:{} Failed to make request.",
          module_name_, session->uid_, session->session_id_);
      bRet = false;

      break;
    }
    session->DoRecord(module_name_ + "::MakeRequest");
    session->current_type_ = module_type_;

  } while (0);

  return bRet;
}

void HTTPModuleWrapper::ProcResponse(
    std::shared_ptr<Session> session,
    std::shared_ptr<HttpClient::Response> response) {
  session->DoRecord(module_name_ + "::ProcResponse");

  bool bRet = true;
  do {
    if (NULL == session->pHandler_) {
      LOG_ERROR(
          "[{}::ProcResponse] uid:{} session_id:{} RPC call object is null in "
          "session.",
          module_name_, session->uid_, session->session_id_);
      bRet = false;

      break;
    }

    LOG_TRACE("[{}::ProcResponse] uid:{} session_id:{} Start process response.",
              module_name_, session->uid_, session->session_id_);

    bRet = session->pHandler_->ProcResponse(session, response, module_type_);
    if (!bRet) {
      LOG_ERROR(
          "[{}::ProcResponse] uid:{} session_id:{} Falied to process response.",
          module_name_, session->uid_, session->session_id_);
      bRet = false;

      break;
    }
  } while (0);

  if (!bRet) {
    session->Final(ErrCode::kProcTerminate);

    return;
  }

  // If all the processor has been done, the session will be finished.
  if (!session->HasNextProcessor()) {
    session->Final(ErrCode::kSuccess);

    return;
  }

  WORKER(Session)->PushMsg(session);

  return;
}
