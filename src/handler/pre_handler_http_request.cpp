/*
* Copyright 2019 <Copyright hobot>
* @brief Feature predict request processor
* @author mengmeng.zhi
* @date 25/Dec/2018
*/

#include <string>

#include "handler/handler_include.h"
#include "package/workflow/http_module_wrapper.h"
#include "message/session.h"
#include "json/json.h"
#include "log/log_module.h"
#include "conf/conf_manager.h"
#include "boost/algorithm/string.hpp"
#include "common/com_func.h"
#include "com_xservice.h"

using namespace XService;

bool PreHandler::makeRequest(std::shared_ptr<Session>& session,
                             RequestInfo& request, ProcType procType) {
  if (!session) {
    LOG_ERROR("[{}::makeRequest] Session is NULL.", name_);
    return false;
  }

  LOG_TRACE(
      "[{}::makeRequest] Enter SearchCall, procType: {}, session_id: {}, ",
      name_, procType, session->session_id_);

  bool bRet = false;
  /*A Pre-handler could only goes into the below protypes to make HTTP request*/
  switch (procType) {
    case kDownloadImage: {
      bRet = makeDownloadRequest(session, request);
      break;
    }
    case kTraceSpace: {
      bRet = makeTraceSpaceRequest(session, request);
      break;
    }
    case kGenerateUrl: {
      bRet = makeGenerateUrlRequest(session, request);
      break;
    }

    default: {
      LOG_ERROR("[{}::makeRequest] session_id: {} Invalid procType: {}.", name_,
                session->session_id_, procType);
      break;
    }
  }

  return bRet;
}

bool PreHandler::makeGenerateUrlRequest(std::shared_ptr<Session>& session,
                                    RequestInfo& request) {
  if (!session) {
    return false;
  }
  
  Json::Value out_jv;
  if (!session->pHandler_->KafkaData_->GetGenerateUrlData(session, out_jv)) {
    return false;
  }
  if (session->person_result_.url != "") {
    Json::Value tmp_jv;
    auto uri = session->person_result_.url;
    auto endpoint = uri.find('/', 0);
    if (endpoint != std::string::npos) {
      tmp_jv[kEndPoint] = uri.substr(0, endpoint);
    } else {
      LOG_ERROR(
          "[{} GenerateUrlModule::MakeRequest]uid:{} session_id:{} no endpoint",
          name_, session->uid_, session->session_id_);
      return false;
    }
    auto bucket = uri.find('/', endpoint + 1);
    if (bucket != std::string::npos) {
      tmp_jv[kBucket] = uri.substr(
          endpoint + 1, bucket - endpoint - 1);
    } else {
      LOG_ERROR("[{} GenerateUrlModule::MakeRequest]uid:{} session_id:{} no bucket",
                name_, session->uid_, session->session_id_);
      return false;
    }
    
    if (bucket + 1 < uri.size()) {
      tmp_jv[kKey] = uri.substr(bucket + 1);
    } else {
      LOG_ERROR("[{} GenerateUrlModule::MakeRequest]uid:{} session_id:{} no key",
                name_, session->uid_, session->session_id_);
      return false;
    }
    out_jv["list"].append(tmp_jv);
  }

  std::string path;
  int ret = CONF_PARSER()->getValue(kS3ProxySection, kGenUrlPath, path);
  if (0 != ret) {
    LOG_ERROR("[GenerateUrlModule::Initialize] path is null");
    return false;
  }
  request.method_ = "POST";
  request.path_ = path;
  request.extra_.emplace_back(
      std::make_pair("Content-Type", "application/json"));
  request.body_ = out_jv.toStyledString();
  LOG_TRACE("[{} GenerateUrlModule::MakeRequest]uid:{} session_id:{} body: {}",
            name_, session->uid_, session->session_id_, request.body_);
  return true;
}


bool PreHandler::makeDownloadRequest(std::shared_ptr<Session>& session,
                                     RequestInfo& request) {
  if (!session) {
    return false;
  }

  if (KafkaData_->getUrlCount() <= 0) {
    LOG_ERROR(
        "[{} DownloadModule::MakeRequest]uid:{} session_id:{} no more url",
        name_, session->uid_, session->session_id_);
    return false;
  }
  session->current_download_url_ = KafkaData_->GetNextUrl();

  if (session->current_download_url_.empty()) {
    LOG_ERROR(
        "[{} DownloadModule::MakeRequest]uid:{} session_id:{} no more url",
        name_, session->uid_, session->session_id_);
    return false;
  }

  Json::Value out_jv;
  auto endpoint = session->current_download_url_.find('/', 0);
  if (endpoint != std::string::npos) {
    out_jv[kEndPoint] = session->current_download_url_.substr(0, endpoint);
  } else {
    LOG_ERROR(
        "[{} DownloadModule::MakeRequest]uid:{} session_id:{} no endpoint",
        name_, session->uid_, session->session_id_);
    return false;
  }
  auto bucket = session->current_download_url_.find('/', endpoint + 1);
  if (bucket != std::string::npos) {
    out_jv[kBucket] = session->current_download_url_.substr(
        endpoint + 1, bucket - endpoint - 1);
  } else {
    LOG_ERROR("[{} DownloadModule::MakeRequest]uid:{} session_id:{} no bucket",
              name_, session->uid_, session->session_id_);
    return false;
  }

  if (bucket + 1 < session->current_download_url_.size()) {
    out_jv[kKey] = session->current_download_url_.substr(bucket + 1);
  } else {
    LOG_ERROR("[{} DownloadModule::MakeRequest]uid:{} session_id:{} no key",
              name_, session->uid_, session->session_id_);
    return false;
  }

  std::string path;
  int ret = CONF_PARSER()->getValue(kS3ProxySection, kPath, path);
  if (0 != ret) {
    LOG_ERROR("[DownloadModule::Initialize] path is null");
    return false;
  }
  request.method_ = "POST";
  request.path_ = path;
  request.extra_.emplace_back(
      std::make_pair("Content-Type", "application/json"));
  request.body_ = out_jv.toStyledString();
  LOG_TRACE("[{} DownloadModule::MakeRequest]uid:{} session_id:{} body: {}",
            name_, session->uid_, session->session_id_, request.body_);
  return true;
}

bool PreHandler::makeTraceSpaceRequest(std::shared_ptr<Session>& session,
                                       RequestInfo& request) {
  if (!session || NULL == session->pHandler_) {
    return false;
  }
  if (session->images_.empty()) {
    LOG_ERROR(
        "[{} PreHandler::makeTraceSpaceRequest]uid:{} session_id {} has no "
        "image",
        name_, session->uid_, session->session_id_);
    session->Final(ErrCode::kDownloadError);
    return false;
  }
  Json::Value out_jv;
  out_jv["dossier_name"] = KafkaData_->ipc_.space_id_;
  out_jv["model_version"] = "";
  out_jv["device_id"] = KafkaData_->ipc_.device_id_;
  out_jv["uid"] = session->session_id_;
  out_jv["ipc_tmst"] = KafkaData_->ipc_.timestamp_;
  out_jv[ktrackid] = KafkaData_->getTrackID();
  out_jv["replay"] = KafkaData_->getReplay();
  out_jv[kImages] =Json::Value(Json::arrayValue);

  for (int i = 0; i < session->images_.size(); ++i) {
    auto& image = session->images_[i];
    Json::Value tmp;
    tmp[kUrl] = image.url_;

    if (KafkaData_->plugin_.enable) {
      if (KafkaData_->plugin_.face_rect_from == 0) {
        std::string rect = KafkaData_->getFaceRectByOrder(i);
        if (rect != "") {
          tmp[kFaceRect] = rect;
        }
      }
      if (KafkaData_->plugin_.face_attr_from == 0) {
        tmp["age"] = KafkaData_->getFaceAgeByOrder(i);
        tmp[kgender] = KafkaData_->getFaceGenderByOrder(i);
      }
      else {
        tmp["age"] = -2;
        tmp[kgender] = -2;
      }
    }
    else {
      std::string rect = KafkaData_->getFaceRectByOrder(i);
      if (rect != "") {
        tmp[kFaceRect] = rect;
      }
      tmp["age"] = KafkaData_->getFaceAgeByOrder(i);
      tmp[kgender] = KafkaData_->getFaceGenderByOrder(i);
    }
    tmp[kImageBase64] = hobotcommon::base64_encode(
        (const unsigned char*)image.img_.c_str(), image.img_.size());
    out_jv[kImages].append(tmp);
  }
  request.method_ = "POST";
  request.path_ = "/api/xtrace/capture/identify?app_id=";
  request.path_ += KafkaData_->ipc_.app_id_;
  request.body_ = out_jv.toStyledString();
  LOG_INFO(
      "[{} PreHandler::makeTraceSpaceRequest] uid:{} session_id:{} request {}",
      name_, session->uid_, session->session_id_, request.body_);
  
  return true;
}

