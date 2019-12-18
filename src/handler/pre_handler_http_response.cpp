/*
* Copyright 2019 <Copyright hobot>
* @brief Person search response processor
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

/*Response section*/
bool PreHandler::ProcResponse(std::shared_ptr<Session>& session,
                              std::shared_ptr<HttpClient::Response>& response,
                              ProcType procType) {
  if (!session) {
    LOG_ERROR("[{}::ProcResponse] Session is NULL.", name_);
    return false;
  }

  bool bRet = false;
  /*A pre-handler could only receive these HTTP response*/
  switch (procType) {
    case kDownloadImage: {
      bRet = ProcImageResponse(session, response);
      break;
    }
    case kTraceSpace: {
      bRet = ProcTraceSpaceResponse(session, response);
      break;
    }
    case kGenerateUrl: {
      bRet = ProcGenerateUrlResponse(session, response);
      break;
    }

    default: {
      LOG_ERROR(
          "[{}::ProcResponse] SessionID: {} Msg_ID: {} Invalid procType: {}.",
          name_, session->session_id_, getMsgID(), procType);
      break;
    }
  }

  return bRet;
}


bool PreHandler::ProcGenerateUrlResponse(
    std::shared_ptr<Session>& session,
    std::shared_ptr<HttpClient::Response>& response) {
  session->DoRecord("GenerateUrlModule::ProcGenerateUrlResponse");
  do {
    if (std::string::npos == response->status_code.find("200")) {
      LOG_TRACE(
          "[GenerateUrlModule::ProcGenerateUrlResponse]uid:{} {} status code {}, body {}",
          KafkaData_->msg_id_, session->session_id_, response->status_code,
          response->content.string());
      return false;
    }
    auto body = response->content.string();
    LOG_TRACE("[PreHandler::ProcGenerateUrlResponse]uid:{} {} rsp {}",
              KafkaData_->msg_id_, session->session_id_, body);
    if (!body.empty()) {
      Json::Value body_jv;
      if (!hobotcommon::parse_json(body, body_jv)) {
        LOG_TRACE("[GenerateUrlModule::ProcGenerateUrlResponse]uid:{} {} parse rsp fail",
                  KafkaData_->msg_id_, session->session_id_);
        break;
      }
      if (!(body_jv.isMember("resultlist") && body_jv["resultlist"].isArray())) {
        break;
      }

      int url_conut = session->pHandler_->KafkaData_->getUrlCount();
      if (session->person_result_.url != "") {
        url_conut++;
      }

      if (body_jv["resultlist"].size() != url_conut){
        break;
      }
      if(!session->pHandler_->KafkaData_->GetGenUrl(session, body_jv)) {
        break;
      }
      
    }
    return true;
  } while (0);
  return false;
}


/*
* @brief Process image download response
* @author mengmeng.zhi
* @date 25/Dec/2018
*/
bool PreHandler::ProcImageResponse(
    std::shared_ptr<Session>& session,
    std::shared_ptr<HttpClient::Response>& response) {
  session->DoRecord("DownloadModule::ProcResponse");
  do {
    if (std::string::npos == response->status_code.find("200")) {
      LOG_TRACE(
          "[DownloadModule::ProcResponse]uid:{} {} status code {}, body {}",
          KafkaData_->msg_id_, session->session_id_, response->status_code,
          response->content.string());
      return false;
    }
    auto body = response->content.string();
    if (!body.empty()) {
      ImageInfo tmp;
      tmp.url_ = session->current_download_url_;
      tmp.img_ = body;
      session->images_.emplace_back(tmp);
    }
  } while (0);

  if (KafkaData_->HasNextUrl()) {
    session->AddChain(ProcType::kDownloadImage);
  }

  return true;
}

bool PreHandler::ProcTraceSpaceResponse(
        std::shared_ptr<Session>& session,
        std::shared_ptr<HttpClient::Response>& response) {
  session->DoRecord("PreHandler::ProcTraceSpaceResponse");

  do {
    if (std::string::npos == response->status_code.find("200")) {
      LOG_TRACE(
          "[PreHandler::ProcTraceSpaceResponse]uid:{} session {} status code "
          "{}",
          KafkaData_->msg_id_, session->session_id_, response->status_code);
      break;
    }
    auto body = response->content.string();
    LOG_TRACE("[PreHandler::ProcTraceSpaceResponse]uid:{} {} rsp {}",
              KafkaData_->msg_id_, session->session_id_, body);
    Json::Value body_jv;
    if (!hobotcommon::parse_json(body, body_jv)) {
      LOG_TRACE("[PreHandler::ProcTraceSpaceResponse]uid:{} {} parse rsp fail",
                KafkaData_->msg_id_, session->session_id_);
      break;
    }
    if (!(body_jv.isMember(kErrorCode) && body_jv[kErrorCode].isInt())) {
      break;
    }
    if (0 != body_jv[kErrorCode].asInt() && -10041 != body_jv[kErrorCode].asInt()) {
      break;
    }

    if (!(body_jv.isMember(kResult) && body_jv[kResult].isObject())){
      break;
    }
    
    auto result = body_jv[kResult];

    //解析track_result
    if (!(result.isMember(kTraceResult) && result[kTraceResult].isObject())) {
      break;
    }
    auto track = result[kTraceResult];
    if (track.isMember("age") && track["age"].isInt()) {
      session->track_result_.age = track["age"].asInt();
    }
    if (track.isMember("gender") && track["gender"].isInt()) {
      session->track_result_.gender = track["gender"].asInt();
    }
    if (track.isMember("score") && track["score"].isDouble()) {
      session->track_result_.score = track["score"].asDouble();
    }

    //解析person_result
    if (!(result.isMember(kPersonResult) && result[kPersonResult].isObject())) {
      break;
    }
    
    auto person = result[kPersonResult];
    if (person.isMember("id") && person["id"].isString()) {
      session->person_result_.id = person["id"].asString();
    }
    if (person.isMember("url") && person["url"].isString()) {
      session->person_result_.url = person["url"].asString();
    }
    if (person.isMember("set_name") && person["set_name"].isString()) {
      session->person_result_.set_name = person["set_name"].asString();
    }
    if (person.isMember("type") && person["type"].isInt()) {
      session->person_result_.type = person["type"].asInt();
    }
    if (person.isMember("is_auto_reg") && person["is_auto_reg"].isInt()) {
      session->person_result_.is_auto_reg = person["is_auto_reg"].asInt();
    }
    if (person.isMember("slimilar") && person["slimilar"].isDouble()) {
      session->person_result_.similar = person["slimilar"].asDouble();
    }
    if (person.isMember("atime") && person["atime"].isInt()) {
      session->person_result_.atime = person["atime"].asInt();
    }


    bool Mage_flag = false;
    if (person.isMember("attribute") && person["attribute"].isArray()) {
      for (int k = 0; k < person["attribute"].size(); k++) {
        PersonResultAttribute tempattributes;
        if (person["attribute"][k].isMember("type") && person["attribute"][k]["type"].isString()) {
          if (person["attribute"][k]["type"].asString() !="") {
             tempattributes.type_ = person["attribute"][k]["type"].asString();
             if (tempattributes.type_ == "manual_age") {
               Mage_flag = true;
             }
          } else {
            continue;
          }
        }

        if (person["attribute"][k].isMember("value") && person["attribute"][k]["value"].isString()) {
          tempattributes.value_ = person["attribute"][k]["value"].asString();
        }

        if (person["attribute"][k].isMember("confidence") && person["attribute"][k]["confidence"].isInt()) {
          tempattributes.confidence_ = person["attribute"][k]["confidence"].asInt();
        }

        if (person["attribute"][k].isMember("mtime") && person["attribute"][k]["mtime"].isInt()) {
          tempattributes.mtime_ = person["attribute"][k]["mtime"].asInt();
        }

        if (person["attribute"][k].isMember("is_manual") && person["attribute"][k]["is_manual"].isBool()) {
          tempattributes.is_manual_ = person["attribute"][k]["is_manual"].asBool();
        }
        session->person_result_.person_attributes_.push_back(tempattributes);
      }
    }    
    if (Mage_flag == false) {
      PersonResultAttribute tempattributes;
      tempattributes.type_ = "manual_age";
      tempattributes.value_ = "-1";
      session->person_result_.person_attributes_.push_back(tempattributes);
    }

    //先将结果保存起来 后面在picwait模块时将结果保存到redis
    session->trace_space_result_ = body;
    return true;
  } while (0);
  return false;
}

