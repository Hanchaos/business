/*
* Copyright 2019 <Copyright hobot>
* @brief Predict call processor
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
#include <list>
#include "com_xservice.h"
#include "workflow/module/module_kafka_output.h"
#include "boost/algorithm/string.hpp"


using namespace XService;

bool BusinessCaptureHandler::makeRequest(std::shared_ptr<Session> &session,
                                         RequestInfo &request, ProcType type) {
  bool bRet = false;
  switch (type) {
    case kSearchFeatureStatic:
      bRet = makeSearchFeatureStaticRequest(session, request);
      break;
    case kSearchFeature:
      bRet = makeSearchFeatureRequest(session, request);
      break;
    case kRegisterPerson:
      bRet = makeRegisterPersonRequest(session, request);
      break;
    default:
      break;
  }
  return bRet;
}

bool BusinessCaptureHandler::makeSearchFeatureStaticRequest(
    std::shared_ptr<Session> &session, RequestInfo &request) {
  if (session->features_.empty()) {
    LOG_ERROR(
        "[{} FeatureSearchStatic::MakeRequest] uid:{} sessionid:{} has no feat",
        name_, session->uid_, session->session_id_);
    return false;
  }
  if (KafkaData_->task_.busi_.category_sets_.empty()) {
    LOG_ERROR(
        "[{} FeatureSearchStatic::MakeRequest] uid:{} sessionid:{} has no set",
        name_, session->uid_, session->session_id_);
    return false;
  }

  Json::Value out_jv;
  out_jv[kSets] = Json::Value(Json::arrayValue);

  for (auto &set : KafkaData_->task_.busi_.category_sets_) {
    Json::Value tmp;
    tmp[kSetName] = set;
    if (!isAutoReg(set)) out_jv[kSets].append(tmp);
  }

  //单个图片也遵循以下规则, 并且整体track为2的情况，已经在前面feature_extra过滤掉了
  /*
  * 0 （可识别不可注册）；
  * 1 （可识别可注册）；
  * 2 （不可识别不可注册）。
  */
  out_jv[kFeatures] = Json::Value(Json::arrayValue);
  std::string s_can_reg_img = session->xfr_result.can_reg_img;
  std::vector<std::string> v_canreg_img;
  boost::split(v_canreg_img, s_can_reg_img, boost::is_any_of(","));
  for (int i = 0; i < v_canreg_img.size(); ++i) {
    int regFlag = atoi(v_canreg_img[i].c_str());
    if (XService::kCanRegist == regFlag 
          || XService::kCanRecognize == regFlag ) {
      if ( (session->features_.find(i) != session->features_.end())) {
        out_jv[kFeatures].append(session->features_[i]);
      }
    }
  }

  // if true, match will update xid person atime
  out_jv[kActiveFlag] = false;
  if (KafkaData_->task_.trace_.auto_reg) {
    out_jv[kActiveFlag] = true;
  }

  /*
  if (session->task_.busi_.threshold_ > 0 && session->task_.busi_.threshold_ <=
  1) {
    Json::Value thres = Json::Value(Json::objectValue);
    thres[kSimilarThres] = session->task_.busi_.threshold_;
    out_jv[kThreshold] = thres;
  }
  */

  request.method_ = "POST";
  request.path_ = "/api/xid/search";
  request.body_ = out_jv.toStyledString();
  // request.extra_.push_back(std::make_pair("log_id", this->uid_));

  // std::string task_type_ = "BusinessAnalyze";
  // request.extra_.push_back(std::make_pair("from", task_type_));

  LOG_INFO(
      "[{} FeatureSearchStatic::MakeRequest] uid:{} session_id:{} request {}",
      name_, session->uid_, session->session_id_, request.body_);

  session->DoRecord("FeatureSearchStatic::MakeRequest");
  session->current_type_ = ProcType::kSearchFeatureStatic;
  return true;
}
bool BusinessCaptureHandler::makeSearchFeatureRequest(
    std::shared_ptr<Session> &session, RequestInfo &request) {
  if (session->features_.empty()) {
    LOG_ERROR(
        "[{} FeatureSearchModule::MakeRequest] uid:{} session_id:{} has no "
        "feat",
        name_, session->uid_, session->session_id_);
    return false;
  }
  if (KafkaData_->task_.busi_.category_sets_.empty()) {
    LOG_ERROR(
        "[{} FeatureSearchModule::MakeRequest] uid:{} session_id:{} has no set",
        name_, session->uid_, session->session_id_);
    return false;
  }

  Json::Value out_jv;
  out_jv[kSets] = Json::Value(Json::arrayValue);
  for (auto &set : KafkaData_->task_.busi_.category_sets_) {
    Json::Value tmp;
    tmp[kSetName] = set;
    if (isAutoReg(set)) out_jv[kSets].append(tmp);
  }

  //单个图片也遵循以下规则, 并且整体track为2的情况，已经在前面feature_extra过滤掉了
  /*
  * 0 （可识别不可注册）；
  * 1 （可识别可注册）；
  * 2 （不可识别不可注册）。
  */
  out_jv[kFeatures] = Json::Value(Json::arrayValue);
  std::string s_can_reg_img = session->xfr_result.can_reg_img;
  std::vector<std::string> v_canreg_img;
  boost::split(v_canreg_img, s_can_reg_img, boost::is_any_of(","));
  for (int i = 0; i < v_canreg_img.size(); ++i) {
    int regFlag = atoi(v_canreg_img[i].c_str());
    if (XService::kCanRegist == regFlag 
          || XService::kCanRecognize == regFlag ) {
      if ( (session->features_.find(i) != session->features_.end())) {
        out_jv[kFeatures].append(session->features_[i]);
      }
    }
  }

  // if true, match will update xid person atime
  out_jv[kActiveFlag] = false;
  if (KafkaData_->task_.trace_.auto_reg) {
    out_jv[kActiveFlag] = true;
  }

  /*
  if (session->task_.busi_.threshold_ > 0 && session->task_.busi_.threshold_ <=
  1) {
    Json::Value thres = Json::Value(Json::objectValue);
    thres[kSimilarThres] = session->task_.busi_.threshold_;
    out_jv[kThreshold] = thres;
  }
  */

  request.method_ = "POST";
  request.path_ = "/api/xid/search";
  request.body_ = out_jv.toStyledString();
  // request.extra_.push_back(std::make_pair("log_id", this->uid_));
  // std::string task_type_ = "BusinessAnalyze";

  // request.extra_.push_back(std::make_pair("from", task_type_));

  LOG_INFO(
      "[{} FeatureSearchModule::MakeRequest] uid:{} session_id:{} request {}",
      name_, session->uid_, session->session_id_, request.body_);
  return true;
}
bool BusinessCaptureHandler::makeRegisterPersonRequest(
    std::shared_ptr<Session> &session, RequestInfo &request) {
  if (session->features_.empty()) {
    LOG_ERROR(
        "[{} PersonRegistModule::MakeRequest] uid:{} session_id:{} has no feat",
        name_, session->uid_, session->session_id_);
    return false;
  }
  if (KafkaData_->task_.busi_.category_sets_.empty()) {
    LOG_ERROR(
        "[{} PersonRegistModule::MakeRequest] uid:{} session_id:{} has no set",
        name_, session->uid_, session->session_id_);
    return false;
  }

  Json::Value out_jv;
  for (auto &set : KafkaData_->task_.busi_.category_sets_) {
    if (isAutoReg(set)) {
      out_jv[kSetName] = set;
      break;
    }
  }

  session->auto_reg_person_id_ = hobotcommon::generate_id(
      "auto_reg_", KafkaData_->ipc_.timestamp_, KafkaData_->ipc_.device_id_,
      KafkaData_->event_.track_id_);
  out_jv[kID] = session->auto_reg_person_id_;
  out_jv[kIsManual] = 0;
  out_jv[kFeatures] = Json::Value(Json::arrayValue);
  std::string s_can_reg_img = session->xfr_result.can_reg_img;
  std::vector<std::string> v_canreg_img;
  boost::split(v_canreg_img, s_can_reg_img, boost::is_any_of(","));
  for (int i = 0; i < v_canreg_img.size(); ++i) {
    if (XService::kCanRegist == atoi(v_canreg_img[i].c_str())) {
      if ((session->images_.size() > i) &&
          (session->features_.find(i) != session->features_.end())) {
        Json::Value temp;
        temp[kUrl] = session->images_[i].url_;
        session->busi_auto_reg_url = session->images_[0].url_;
        temp[kFeature] = session->features_[i];
        out_jv[kFeatures].append(temp);
      }
    }
  }

  out_jv[kAttribute] = Json::Value(Json::arrayValue);
  Json::Value attr_temp;
  attr_temp[kType] = "age";
  attr_temp[kValue] = session->xfr_result.age;
  out_jv[kAttribute].append(attr_temp);

  attr_temp[kType] = "gender";
  attr_temp[kValue] = session->xfr_result.gender;
  out_jv[kAttribute].append(attr_temp);

  request.method_ = "POST";
  request.path_ = "/api/xid/id/create";
  request.body_ = out_jv.toStyledString();

  LOG_INFO(
      "[{} PersonRegistModule::MakeRequest] uid:{} session_id:{} request {}",
      name_, session->uid_, session->session_id_, request.body_);
  return true;
}

bool BusinessCaptureHandler::ProcResponse(
    std::shared_ptr<Session> &session,
    std::shared_ptr<HttpClient::Response> &response, ProcType type) {
  bool bRet = false;
  switch (type) {
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
    default:
      return false;
      break;
  }
  return bRet;
}

bool BusinessCaptureHandler::ProcSearchFeatureStaticResponse(
    std::shared_ptr<Session> &session,
    std::shared_ptr<HttpClient::Response> &response) {
  bool is_success = false;

  do {
    if (std::string::npos == response->status_code.find("200")) {
      LOG_ERROR(
          "[{} FeatureSearchStatic::ProcResponse] uid:{} session_id:{} status "
          "code {}",
          name_, session->uid_, session->session_id_, response->status_code);
      break;
    }
    auto body = response->content.string();
    Json::Value body_jv;
    if (!hobotcommon::parse_json(body, body_jv)) {

      LOG_ERROR(
          "[{} FeatureSearchStatic::ProcResponse] uid:{} session_id:{} parse "
          "rsp fail",
          name_, session->uid_, session->session_id_);

      break;
    }

    LOG_INFO(
        "[{} FeatureSearchStatic::ProcResponse] uid:{} session_id:{} rsp {}",
        name_, session->uid_, session->session_id_, body);

    if (body_jv.isMember(kErrorCode) && body_jv[kErrorCode].isInt()) {

      LOG_TRACE(
          "[{} FeatureSearchStatic::ProcResponse] uid:{} session_id:{} errcode "
          "{}",
          name_, session->uid_, session->session_id_,
          body_jv[kErrorCode].asInt());

      if (0 == body_jv[kErrorCode].asInt()) {
        is_success = true;
        if (body_jv.isMember(kResult) && body_jv[kResult].isObject()) {
          if (body_jv[kResult].isMember(kItems) &&
              body_jv[kResult][kItems].isArray()) {
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
      }
    }
  } while (0);
  if (session->is_static_matched_ == false) {
    session->matched_infos_.clear();
  }

  session->is_xid_matched_ = session->is_static_matched_;

  LOG_TRACE(
      "[{} FeatureSearchStatic::ProcResponse] uid:{} session_id:{} vip match "
      "result {},is_autoregister_ {}",
      name_, session->uid_, session->session_id_,
      (int)(session->is_static_matched_), session->is_autoregister_);

  if (is_success) {
    return true;
  } else {
    return false;
  }
}
bool BusinessCaptureHandler::ProcSearchFeatureResponse(
    std::shared_ptr<Session> &session,
    std::shared_ptr<HttpClient::Response> &response) {
  bool is_success = false;
  do {
    if (std::string::npos == response->status_code.find("200")) {
      LOG_ERROR(
          "[{} FeatureSearchModule::ProcResponse] uid:{} session_id:{} {} "
          "status code {}",
          name_, session->uid_, session->session_id_, response->status_code);
      break;
    }
    auto body = response->content.string();
    Json::Value body_jv;
    if (!hobotcommon::parse_json(body, body_jv)) {

      LOG_ERROR(
          "[{} FeatureSearchModule::ProcResponse] uid:{} session_id:{} parse "
          "rsp fail",
          name_, session->uid_, session->session_id_);
      break;
    }

    LOG_INFO(
        "[{} FeatureSearchModule::ProcResponse] uid:{} session_id:{} rsp {}",
        name_, session->uid_, session->session_id_, body);

    if (body_jv.isMember(kErrorCode) && body_jv[kErrorCode].isInt()) {

      LOG_TRACE(
          "[{} FeatureSearchModule::ProcResponse] uid:{} session_id:{} errcode "
          "{}",
          name_, session->uid_, session->session_id_,
          body_jv[kErrorCode].asInt());

      if (0 == body_jv[kErrorCode].asInt()) {
        is_success = true;
        if (body_jv.isMember(kResult) && body_jv[kResult].isObject()) {
          if (body_jv[kResult].isMember(kItems) &&
              body_jv[kResult][kItems].isArray()) {
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
      }
    }
  } while (0);
  if (session->is_xid_matched_ == false) {
    session->matched_infos_.clear();
  }

  LOG_TRACE(
      "[{} FeatureSearchModule::ProcResponse] uid:{} session_id:{} xid match "
      "result {},is_autoregister_ {}",
      name_, session->uid_, session->session_id_,
      (int)(session->is_xid_matched_), session->is_autoregister_);

  if (is_success) {
    return true;
  } else {
    return false;
  }
}
bool BusinessCaptureHandler::ProcRegisterPersonResponse(
    std::shared_ptr<Session> &session,
    std::shared_ptr<HttpClient::Response> &response) {
  bool is_success = false;
  do {
    if (std::string::npos == response->status_code.find("200")) {
      LOG_ERROR(
          "[{} PersonRegistModule::ProcResponse] uid:{} session_id:{} status "
          "code {}",
          name_, session->uid_, session->session_id_, response->status_code);
      break;
    }
    auto body = response->content.string();

    LOG_INFO(
        "[{} PersonRegistModule::ProcResponse] uid:{} sesssion_id:{} rsp {}",
        name_, session->uid_, session->session_id_, body);

    Json::Value body_jv;
    if (!hobotcommon::parse_json(body, body_jv)) {

      LOG_ERROR(
          "[{} PersonRegistModule::ProcResponse] uid:{} session_id:{} parse "
          "rsp fail",
          name_, session->uid_, session->session_id_);
      break;
    }

    if (body_jv.isMember(kErrorCode) && body_jv[kErrorCode].isInt()) {
      LOG_TRACE(
          "[{} PersonRegistModule::ProcResponse] uid:{} session_id:{} errcode "
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
        "[{} PersonRegistModule::ProcResponse] uid:{} session_id:{} {} "
        "register failed",
        name_, session->uid_, session->session_id_);
    return false;
  }
}

bool BusinessCaptureHandler::makeKafkaRequest(
    std::shared_ptr<Session> &session,
    std::list<KafkaOutputInfo> &ouput_info_list) {

  Json::Value out_jv;
  out_jv["uid"] = KafkaData_->task_.uid_;
  out_jv["device_id"] = KafkaData_->task_.device_id_;
  out_jv["task_id"] = KafkaData_->task_.task_id_;
  out_jv["extra"] = KafkaData_->task_.extra_;
  out_jv["event_time"] = KafkaData_->event_.event_time_;
  out_jv["send_time"] = KafkaData_->event_.send_time_;
  out_jv["line_id"] = KafkaData_->event_.line_id_;
  out_jv["event_type"] = KafkaData_->event_.event_type_;
  out_jv["msg_type"] = KafkaData_->event_.msg_type_;
  out_jv["track_id"] = KafkaData_->event_.track_id_;
  out_jv["capture_uris"] = Json::Value(Json::arrayValue);
  for (auto &captureface : KafkaData_->event_.capture_uris_) {
    out_jv["capture_uris"].append(captureface.url_);
  }
  out_jv["replay"] = KafkaData_->event_.replay_;
  out_jv["id"] = "";
  out_jv["score"] = -1;
  out_jv["match_url"] = "";
  out_jv["set_name"] = "";
  out_jv["age"] = session->xfr_result.age;
  out_jv["gender"] = session->xfr_result.gender;
  if (session->is_xid_matched_) {
    MatchPersonInfo match_person;
    match_person.id = "";
    match_person.similar = 0;
    match_person.set_name = "";
    double max_similarity = 0.0;
    for (std::list<MatchPersonInfo>::iterator iter =
             session->matched_infos_.begin();
         iter != session->matched_infos_.end(); iter++) {
      if (iter->similar > max_similarity) {
        max_similarity = iter->similar;
        match_person = *iter;
      }
    }
    out_jv["id"] = match_person.id;
    out_jv["score"] = (float)match_person.similar;
    if (!match_person.urls.empty()) {
      out_jv["match_url"] = match_person.urls.front();
    }
    out_jv["set_name"] = match_person.set_name;
  } else {
    if (session->is_auto_reg_success_) {
      out_jv["id"] = session->auto_reg_person_id_;
      out_jv["set_name"] = KafkaData_->task_.trace_.name_;
      out_jv["score"] = 1.0;
      out_jv["match_url"] = session->busi_auto_reg_url;
    }
  }

  KafkaOutputInfo tmp;
  tmp.data = out_jv.toStyledString();
  tmp.topic = KafkaData_->task_.kafka_topic_;
  ouput_info_list.push_back(tmp);
  LOG_INFO(
      "[BusinessCaptureHandler::makeKafkaRequest]uid:{} session_id:{} kafka "
      "topic:{} output:{}",
      session->uid_, session->session_id_, tmp.topic, tmp.data);

  return true;
}

bool BusinessCaptureHandler::isAutoReg(std::string set_name) {

  std::string str_autoreg = "autoreg";
  int cut_length = set_name.length() - str_autoreg.length();
  if (cut_length > 0) {
    std::string str_postfix = set_name.substr(cut_length);
    if (str_autoreg == str_postfix) return true;
  }
  return false;
}

bool BusinessCaptureHandler::getAutoRegSet(std::list<std::string> &set_list) {

  bool is_has_auto_reg_set = false;
  for (auto &set : KafkaData_->task_.busi_.category_sets_) {
    if (isAutoReg(set)) {
      set_list.push_back(set);
      is_has_auto_reg_set = true;
    }
  }
  return is_has_auto_reg_set;
}
