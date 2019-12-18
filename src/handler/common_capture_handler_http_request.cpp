/*
* Copyright 2019 <Copyright hobot>
* @brief Feature predict request processor
* @author mengmeng.zhi
* @date 25/Dec/2018
*/

#include <string>

#include "package/workflow/http_module_wrapper.h"
#include "message/session.h"
#include "json/json.h"
#include "log/log_module.h"
#include "conf/conf_manager.h"
#include "boost/algorithm/string.hpp"
#include "common/com_func.h"
#include <workflow/worker.hpp>
#include "handler/handler_include.h"
#include "com_xservice.h"
#include "boost/algorithm/string.hpp"

using namespace XService;

bool CommonCaptureHandler::makeRequest(std::shared_ptr<Session> &session,
                                       RequestInfo &request,
                                       ProcType procType) {
  if (!session || !session->pHandler_) {
    LOG_ERROR("[{}::makeRequest] Session is NULL.", name_);
    return false;
  }

  bool bRet = false;
  /*A PredictCall RPC could only goes into the below protypes to make HTTP
   * request*/
  switch (procType) {
    case kSearchFeatureStatic: {
      bRet = makeSearchFeatureStaticRequest(session, request);
      break;
    }
    case kSearchFeature: {
      bRet = makeSearchFeatureRequest(session, request);
      break;
    }
    case kRegisterPerson: {
      bRet = makeRegisterPersonRequest(session, request);
      break;
    }

    default: {
      LOG_ERROR("[{}::makeRequest] uid:{} session_id: {} Invalid procType: {}.",
                name_, session->uid_, session->session_id_, procType);

      break;
    }
  }

  return bRet;
}

bool CommonCaptureHandler::makeSearchFeatureStaticRequest(
    std::shared_ptr<Session> &session, RequestInfo &request) {
  if (session->features_.empty()) {

    LOG_ERROR(
        "[{} StaticFeatureSearchModule::MakeRequest] uid:{} session_id:{} has "
        "no feat",
        name_, session->uid_, session->session_id_);

    return false;
  }
  if (KafkaData_->task_.trace_.category_sets_.empty()) {
    LOG_ERROR(
        "[{} StaticFeatureSearchModule::MakeRequest] uid:{} session_id:{} has "
        "no set",
        name_, session->uid_, session->session_id_);

    return false;
  }

  Json::Value out_jv;
  out_jv[kSets] = Json::Value(Json::arrayValue);
  for (auto &set : KafkaData_->task_.trace_.category_sets_) {
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

  out_jv[kActiveFlag] = true;
  /*if (XService::MATCH_THRES_NOT_SET != session->task_.trace_.match_threshold_)
  {
      Json::Value thres = Json::Value(Json::objectValue);
      thres[kSimilarThres] = session->task_.trace_.match_threshold_;
      out_jv[kThreshold] = thres;
  }*/
  request.method_ = "POST";
  request.path_ = "/api/xid/search";
  request.body_ = out_jv.toStyledString();
  request.extra_.push_back(std::make_pair("log_id", KafkaData_->msg_id_));
  std::string task_type_ = "Trace";
  request.extra_.push_back(std::make_pair("from", task_type_));
  LOG_INFO(
      "[{} StaticFeatureSearchModule::MakeRequest]uid:{} session_id:{} request "
      "{}",
      name_, session->uid_, session->session_id_, request.body_);
  return true;
}

bool CommonCaptureHandler::makeSearchFeatureRequest(
    std::shared_ptr<Session> &session, RequestInfo &request) {
  if (session->features_.empty()) {
    LOG_ERROR(
        "[{} FeatureSearchModule::MakeRequest]uid:{} session_id:{} has no feat",
        name_, session->uid_, session->session_id_);
    return false;
  }
  if (KafkaData_->task_.trace_.category_sets_.empty()) {
    LOG_ERROR(
        "[{} FeatureSearchModule::MakeRequest]uid:{} session_id {} has no set",
        name_, session->uid_, session->session_id_);
    return false;
  }

  Json::Value out_jv;
  out_jv[kSets] = Json::Value(Json::arrayValue);
  for (auto &set : KafkaData_->task_.trace_.category_sets_) {
    Json::Value tmp;
    tmp[kSetName] = set;
    // 如果不是安防的话，商业前面的search已经检索过静态库，不用再检索
    if (KafkaData_->task_.trace_.group_id_ == kCityGroupID) {
      out_jv[kSets].append(tmp);
    } else if (isAutoReg(set)) {
      out_jv[kSets].append(tmp);
    }
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

  out_jv[kActiveFlag] = true;
  /*if (XService::MATCH_THRES_NOT_SET != session->task_.trace_.match_threshold_)
  {
    Json::Value thres = Json::Value(Json::objectValue);
    thres[kSimilarThres] = session->task_.trace_.match_threshold_;
    out_jv[kThreshold] = thres;
  }*/

  request.method_ = "POST";
  request.path_ = "/api/xid/search";
  request.body_ = out_jv.toStyledString();
  request.extra_.push_back(std::make_pair("log_id", KafkaData_->msg_id_));
  request.extra_.push_back(std::make_pair("from", "Trace"));
  LOG_INFO(
      "[{} FeatureSearchModule::MakeRequest]uid:{} session_id:{} request {}",
      name_, session->uid_, session->session_id_, request.body_);
  return true;
}

bool CommonCaptureHandler::makeRegisterPersonRequest(
    std::shared_ptr<Session> &session, RequestInfo &request) {
  if (session->features_.empty()) {
    LOG_ERROR(
        "[{} PersonRegistModule::MakeRequest]uid:{} session_id:{} has no feat",
        name_, session->uid_, session->session_id_);
    return false;
  }
  if (KafkaData_->task_.trace_.name_.empty()) {
    LOG_ERROR(
        "[{} PersonRegistModule::MakeRequest]uid:{} session_id:{} has no "
        "auto_set",
        name_, session->uid_, session->session_id_);
    return false;
  }

  session->auto_reg_person_id_ = hobotcommon::generate_id(
      "auto_reg_", KafkaData_->ipc_.timestamp_, KafkaData_->ipc_.device_id_,
      KafkaData_->capture_.person_.track_id_);
  Json::Value out_jv;
  out_jv[kSetName] = KafkaData_->task_.trace_.name_;
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
      "[{} PersonRegistModule::MakeRequest]uid:{} session_id:{} request {}",
      name_, session->uid_, session->session_id_, request.body_);
  return true;
}
