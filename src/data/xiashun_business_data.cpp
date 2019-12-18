#include "xiashun_business_data.h"
#include "com_xservice.h"
#include <list>
#include "DataDefine.h"
#include <string>
#include "workflow/module_define.h"
#include "json/json.h"
#include "common/com_func.h"
#include "session.h"
#include "workflow/module/module_kafka_output.h"
#include "conf/conf_manager.h"

using namespace XService;

bool xiashunBusinessData::makeProcessChain(std::list<ProcType> &procList) {
  procList.push_back(kBusinessSave);
  return true;
}

bool xiashunBusinessData::parse(void) {

  Json::Value data_jv;
  if (total_jv_.isMember(kdata)) {
    if (total_jv_[kdata].isObject()) {
      data_jv = total_jv_[kdata];
    } else if (total_jv_[kdata].isString()) {
      if (!hobotcommon::parse_json(total_jv_[kdata].asString(), data_jv))
        return false;
    } else {
      return false;
    }
  } else {
    return false;
  }

  if (total_jv_.isMember(kmsgid) && total_jv_[kmsgid].isString()) {
    msg_id_ = total_jv_[kmsgid].asString();
  }

  Json::Value payload_jv;
  if (data_jv.isMember("payload") && data_jv["payload"].isString()) {
    std::string payload = data_jv["payload"].asString();
    if (!hobotcommon::parse_json(payload, payload_jv)) {
      LOG_ERROR("parse payload fail");
      return false;
    }
  } else {
    return false;
  }
  //解析ipc信息
  if (!parseIpcInfo(data_jv)) {
    return false;
  }
  //解析商业event
  if (!ParseEvent(payload_jv)) {
    return false;
  }

  return true;
}

bool xiashunBusinessData::ParseEvent(Json::Value &payload_jv) {
  Json::Value event_jv;
  if (payload_jv.isMember(kevent) && payload_jv[kevent].isObject()) {
    event_jv = payload_jv[kevent];
  } else {
    return false;
  }
  EventInfo &event = event_;
  event_.msg_type_ = 0;
  if (event_jv.isMember(kreplay) && event_jv[kreplay].isInt()) {
    event.replay_ = event_jv[kreplay].asInt();
  }
  if (event_jv.isMember(ktime) && event_jv[ktime].isInt64()) {
    event.event_time_ = event_jv[ktime].asInt64();
  }
  if (event_jv.isMember(klineid) && event_jv[klineid].isInt()) {
    event.line_id_ = event_jv[klineid].asInt();
  }
  if (event_jv.isMember(kflowtype) && event_jv[kflowtype].isInt()) {
    event.event_type_ = event_jv[kflowtype].asInt();
  }
  if (event_jv.isMember(kpersonid) && event_jv[kpersonid].isInt64()) {
    event.track_id_ = event_jv[kpersonid].asInt64();
  }
  return true;
}


bool xiashunBusinessData::GetSaveMongoData(std::shared_ptr<Session> session) {
  session->saveMongoData_.event_type_ = event_.event_type_;
  session->saveMongoData_.msg_type_ = event_.msg_type_;
  session->saveMongoData_.event_time_ = event_.event_time_;
  session->saveMongoData_.track_id_ = event_.track_id_;
  return true;
}



