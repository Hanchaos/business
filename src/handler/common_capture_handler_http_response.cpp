/*
* Copyright 2019 <Copyright hobot>
* @brief Person search response processor
* @author mengmeng.zhi
* @date 25/Dec/2018
*/

#include <string>

#include "handler/common_capture_handler.h"
#include "package/workflow/http_module_wrapper.h"
#include "message/session.h"
#include "json/json.h"
#include "log/log_module.h"
#include "conf/conf_manager.h"
#include "boost/algorithm/string.hpp"
#include "common/com_func.h"
#include "com_xservice.h"

using namespace XService;

/*Response section*/
bool CommonCaptureHandler::ProcResponse(
    std::shared_ptr<Session> &session,
    std::shared_ptr<HttpClient::Response> &response, ProcType procType) {
  if (!session || !session->pHandler_) {
    LOG_ERROR("[{}::ProcResponse] Session is NULL.", name_);
    return false;
  }

  bool bRet = false;
  /*A PredictCall RPC could only receive these HTTP response*/
  switch (procType) {
    case kSearchFeatureStatic: {
      bRet = ProcSearchFeatureStaticResponse(session, response);
      break;
    }
    case kSearchFeature: {
      bRet = ProcSearchFeatureResponse(session, response);
      break;
    }
    case kRegisterPerson: {
      bRet = ProcRegisterPersonResponse(session, response);
      break;
    }

    default: {
      LOG_ERROR("[{}::ProcResponse] uid:{} session_id:{} Invalid procType: {}.",
                name_, session->uid_, session->session_id_, procType);
      break;
    }
  }

  return bRet;
}
bool CommonCaptureHandler::ProcSearchFeatureStaticResponse(
    std::shared_ptr<Session> &session,
    std::shared_ptr<HttpClient::Response> &response) {
  do {
    if (std::string::npos == response->status_code.find("200")) {
      LOG_ERROR(
          "[{} StaticFeatureSearchModule::ProcResponse]uid:{} session_id:{} "
          "status code {}",
          name_, session->uid_, session->session_id_, response->status_code);
      return false;
    }
    auto body = response->content.string();
    Json::Value body_jv;
    if (!hobotcommon::parse_json(body, body_jv)) {
      LOG_ERROR(
          "[{} StaticFeatureSearchModule::ProcResponse]uid:{} session_id:{} "
          "parse rsp fail",
          name_, session->uid_, session->session_id_);
      return false;
    }

    LOG_TRACE(
        "[{} StaticFeatureSearchModule::ProcResponse]uid:{} session_id:{} rsp "
        "{}",
        name_, session->uid_, session->session_id_, body);

    if (body_jv.isMember(kErrorCode) && body_jv[kErrorCode].isInt()) {
      LOG_INFO(
          "[{} StaticFeatureSearchModule::ProcResponse]uid:{} session_id:{} "
          "errcode {}",
          name_, session->uid_, session->session_id_,
          body_jv[kErrorCode].asInt());
      if (0 == body_jv[kErrorCode].asInt()) {
        if (body_jv.isMember(kResult) && body_jv[kResult].isObject()) {
          if (body_jv[kResult].isMember(kItems) &&
              body_jv[kResult][kItems].isArray()) {
            LOG_INFO(
                "[{} StaticFeatureSearchModule::ProcResponse] uid:{} "
                "session_id:{} top1_item:{}",
                name_, session->uid_, session->session_id_,
                body_jv[kResult][kItems][0].toStyledString());
            for (auto i = 0; i < body_jv[kResult][kItems].size(); ++i) {
              auto item = body_jv[kResult][kItems][i];
              MatchPersonInfo info;
              if (item.isMember(kSimilar) && item[kSimilar].isDouble()) {
                info.similar = item[kSimilar].asDouble();
              }
              if (item.isMember(kDistance) && item[kDistance].isDouble()) {
                info.distance = item[kDistance].asDouble();
              }
              if (item.isMember(kID) && item[kID].isString()) {
                info.id = item[kID].asString();
              }
              if (item.isMember(kSetName) && item[kSetName].isString()) {
                info.set_name = item[kSetName].asString();
              }
              if (item.isMember(kUrls) && item[kUrls].isArray()) {
                for (auto i = 0; i < item[kUrls].size(); ++i) {
                  info.urls.push_back(item[kUrls][i].asString());
                }
              }

              if (item.isMember(kAttribute) && item[kAttribute].isArray()) {
                for (auto j = 0; j < item[kAttribute].size(); ++j) {
                  auto attr_ = item[kAttribute][j];
                  MatchPersonInfo::Attribute attribute;
                  attribute.type = attr_[kType].asString();
                  attribute.value = attr_[kValue].asString();
                  info.attrs.push_back(attribute);
                }
              }

              if (item.isMember(kATime) && item[kATime].isString()) {
                info.atime = item[kATime].asString();
              }

              if (item.isMember(kCTime) && item[kCTime].isString()) {
                info.ctime = item[kCTime].asString();
              }

              session->matched_infos_.push_back(info);
            }
          }
          if (body_jv[kResult].isMember(kIsMatched)) {
            session->is_static_matched_ = body_jv[kResult][kIsMatched].asBool();
            if (session->matched_infos_.empty()) {
              session->is_static_matched_ = false;
            }
          }
          if (body_jv[kResult].isMember(kAutoregisterFlag)) {
            session->is_autoregister_ =
                body_jv[kResult][kAutoregisterFlag].asBool();
          }
        }
      } else
        return false;
    } else
      return false;
  } while (0);
  if (!session->is_static_matched_) {
    session->matched_infos_.clear();
  }

  session->is_xid_matched_ = session->is_static_matched_;

  LOG_INFO(
      "[{} StaticFeatureSearchModule::ProcResponse] uid:{} session_id:{} "
      "static library match result {},is_autoregister_ {}",
      name_, session->uid_, session->session_id_,
      (int)(session->is_static_matched_), (session->is_autoregister_));
  return true;
}
bool CommonCaptureHandler::ProcSearchFeatureResponse(
    std::shared_ptr<Session> &session,
    std::shared_ptr<HttpClient::Response> &response) {
  do {
    if (std::string::npos == response->status_code.find("200")) {
      LOG_ERROR(
          "[{} FeatureSearchModule::ProcResponse] uid:{} session_id:{} status "
          "code {}",
          name_, session->uid_, session->session_id_, response->status_code);
      return false;
    }
    auto body = response->content.string();
    Json::Value body_jv;
    if (!hobotcommon::parse_json(body, body_jv)) {
      LOG_ERROR(
          "[FeatureSearchModule::ProcResponse] uid:{} session_id:{} parse rsp "
          "fail",
          name_, session->uid_, session->session_id_);
      return false;
    }
    LOG_TRACE(
        "[{} FeatureSearchModule::ProcResponse] uid:{} session_id:{} rsp {}",
        name_, session->uid_, session->session_id_, body);

    if (body_jv.isMember(kErrorCode) && body_jv[kErrorCode].isInt()) {
      LOG_INFO(
          "[{} FeatureSearchModule::ProcResponse] uid:{} session_id:{} errcode "
          "{}",
          name_, session->uid_, session->session_id_,
          body_jv[kErrorCode].asInt());
      if (0 == body_jv[kErrorCode].asInt()) {
        if (body_jv.isMember(kResult) && body_jv[kResult].isObject()) {
          if (body_jv[kResult].isMember(kItems) &&
              body_jv[kResult][kItems].isArray()) {
            LOG_INFO(
                "[{} FeatureSearchModule::ProcResponse] uid:{} session_id:{} "
                "top1_items:{}",
                name_, session->uid_, session->session_id_,
                body_jv[kResult][kItems][0].toStyledString());
            for (auto i = 0; i < body_jv[kResult][kItems].size(); ++i) {
              auto item = body_jv[kResult][kItems][i];
              MatchPersonInfo info;
              if (item.isMember(kSimilar) && item[kSimilar].isDouble()) {
                info.similar = item[kSimilar].asDouble();
              }
              if (item.isMember(kDistance) && item[kDistance].isDouble()) {
                info.distance = item[kDistance].asDouble();
              }
              if (item.isMember(kID) && item[kID].isString()) {
                info.id = item[kID].asString();
              }
              if (item.isMember(kSetName) && item[kSetName].isString()) {
                info.set_name = item[kSetName].asString();
              }
              if (item.isMember(kUrls) && item[kUrls].isArray()) {
                for (auto i = 0; i < item[kUrls].size(); ++i) {
                  info.urls.push_back(item[kUrls][i].asString());
                }
              }

              if (item.isMember(kAttribute) && item[kAttribute].isArray()) {
                for (auto j = 0; j < item[kAttribute].size(); ++j) {
                  auto attr_ = item[kAttribute][j];
                  MatchPersonInfo::Attribute attribute;
                  attribute.type = attr_[kType].asString();
                  attribute.value = attr_[kValue].asString();
                  info.attrs.push_back(attribute);
                }
              }

              if (item.isMember(kATime) && item[kATime].isString()) {
                info.atime = item[kATime].asString();
              }

              if (item.isMember(kCTime) && item[kCTime].isString()) {
                info.ctime = item[kCTime].asString();
              }

              session->matched_infos_.push_back(info);
            }
          }
          if (body_jv[kResult].isMember(kIsMatched)) {
            session->is_xid_matched_ = body_jv[kResult][kIsMatched].asBool();
            if (session->matched_infos_.empty()) {
              session->is_xid_matched_ = false;
            }
          }
          if (body_jv[kResult].isMember(kAutoregisterFlag)) {
            session->is_autoregister_ =
                body_jv[kResult][kAutoregisterFlag].asBool();
          }
        }
      } else
        return false;
    } else
      return false;
  } while (0);
  LOG_INFO(
      "[{} FeatureSearchModule::ProcResponse]uid:{} session_id:{} xid match "
      "result {},is_autoregister_ {}",
      name_, session->uid_, session->session_id_,
      (int)(session->is_xid_matched_), (session->is_autoregister_));
  return true;
}
bool CommonCaptureHandler::ProcRegisterPersonResponse(
    std::shared_ptr<Session> &session,
    std::shared_ptr<HttpClient::Response> &response) {
  bool is_success = false;
  do {
    if (std::string::npos == response->status_code.find("200")) {
      LOG_ERROR(
          "[{} PersonRegistModule::ProcResponse]uid:{} session_id:{} status "
          "code {}",
          name_, session->uid_, session->session_id_, response->status_code);
      break;
    }
    auto body = response->content.string();
    LOG_TRACE(
        "[{} PersonRegistModule::ProcResponse]uid:{} session_id:{} rsp {}",
        name_, session->uid_, session->session_id_, body);
    Json::Value body_jv;
    if (!hobotcommon::parse_json(body, body_jv)) {
      LOG_ERROR(
          "[{} PersonRegistModule::ProcResponse]uid:{} session_id:{} parse rsp "
          "fail",
          name_, session->uid_, session->session_id_);
      break;
    }

    if (body_jv.isMember(kErrorCode) && body_jv[kErrorCode].isInt()) {
      LOG_INFO(
          "[{} PersonRegistModule::ProcResponse]uid:{} session_id:{} errcode "
          "{}",
          name_, session->uid_, session->session_id_,
          body_jv[kErrorCode].asInt());
      if (0 == body_jv[kErrorCode].asInt() ||
          ErrCode::kIDExisted == body_jv[kErrorCode].asInt()) {
        is_success = true;
      }
    }
  } while (0);

  if (is_success) {
    LOG_TRACE(
        "[{} PersonRegistModule::ProcResponse] uid:{} session_id:{} register "
        "success",
        name_, session->uid_, session->session_id_);
    session->is_auto_reg_success_ = true;
    return true;
  } else {
    LOG_ERROR(
        "[{} PersonRegistModule::ProcResponse]uid:{} session_id:{} register "
        "failed",
        name_, session->uid_, session->session_id_);
    session->auto_reg_person_id_ = "";
    return false;
  }
}
