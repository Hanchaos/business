/*
* Copyright 2019 <Copyright hobot>
* @brief Predict call processor
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
#include "handler/pre_handler.h"
#include "com_xservice.h"

using namespace XService;

bool PreHandler::makeKafkaRequest(std::shared_ptr<Session> &session,
                                  std::list<KafkaOutputInfo> &ouput_info_list) {
  return KafkaData_->makeKafkaRequest(session, ouput_info_list);
}



bool PreHandler::getPersonResult(std::shared_ptr<Session> &session, std::string &pic_result) {

  Json::Value result_jv;
  do {
    if (!hobotcommon::parse_json(pic_result, result_jv)) {
      LOG_TRACE("[PreHandler::getPersonResult]uid:{} {} parse pic_result fail",
                KafkaData_->msg_id_, session->session_id_);
      break;
    }
    if (!(result_jv.isMember(kErrorCode) && result_jv[kErrorCode].isInt())) {
      break;
    }
    if (0 != result_jv[kErrorCode].asInt()) {
      break;
    }

    if (!(result_jv.isMember(kResult) && result_jv[kResult].isObject())){
      break;
    }

    auto result = result_jv[kResult];

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
    return true;
  } while (0);
  return false;
}

