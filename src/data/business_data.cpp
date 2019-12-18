#include "business_data.h"
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

bool BusinessData::makeProcessChain(std::list<ProcType> &procList) {
  if (event_.msg_type_ == MsgType::BusinessCapture) {
    if (from == kSrcFromRedis) {
      procList.push_back(kGetPlugin);
      procList.push_back(kBusinessSave);
      procList.push_back(kGenerateUrl);
      procList.push_back(kOutputKafka);

    } else {    
      procList.push_back(kPicWait);
      procList.push_back(kGetPlugin);
      procList.push_back(kBusinessSave);
      procList.push_back(kGenerateUrl);
      procList.push_back(kOutputKafka);
    }
  } else {
    //商业客流和属性数据预处理只有kRedis
     procList.push_back(kGetPlugin);
     procList.push_back(kBusinessSave);
  }
  return true;
}

bool BusinessData::parse(void) {

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

bool BusinessData::ParseEvent(Json::Value &payload_jv) {
  Json::Value event_jv;
  if (payload_jv.isMember(kevent) && payload_jv[kevent].isObject()) {
    event_jv = payload_jv[kevent];
  } else {
    return false;
  }
  EventInfo &event = event_;
  if (event_jv.isMember(kreplay) && event_jv[kreplay].isInt()) {
    event.replay_ = event_jv[kreplay].asInt();
  }
  if (event_jv.isMember(kgroupid) && event_jv[kgroupid].isString()) {
    event.group_id_ = event_jv[kgroupid].asString();
  }
  if (event_jv.isMember(kextra) && event_jv[kextra].isString()) {
    event.extra_ = event_jv[kextra].asString();
  }
  if (event_jv.isMember(keventtime) && event_jv[keventtime].isInt64()) {
    event.event_time_ = event_jv[keventtime].asInt64();
  }
  if (event_jv.isMember(ksendtime) && event_jv[ksendtime].isInt64()) {
    event.send_time_ = event_jv[ksendtime].asInt64();
  }
  if (event_jv.isMember(klineid) && event_jv[klineid].isInt()) {
    event.line_id_ = event_jv[klineid].asInt();
  }
  if (event_jv.isMember(keventtype) && event_jv[keventtype].isInt()) {
    event.event_type_ = event_jv[keventtype].asInt();
  }
  if (event_jv.isMember(kmsgtype) && event_jv[kmsgtype].isInt()) {
    event.msg_type_ = event_jv[kmsgtype].asInt();
    plugin_.datatype = event.msg_type_ + 2;
  }
  if (event_jv.isMember(ktrackid) && event_jv[ktrackid].isInt64()) {
    event.track_id_ = event_jv[ktrackid].asInt64();
  }
  if (event_jv.isMember(kcaptureuris) && event_jv[kcaptureuris].isArray()) {
    event.capture_uris_.clear();
    for (int i = 0; i < event_jv[kcaptureuris].size(); ++i) {
      auto face = event_jv[kcaptureuris][i];
      EventInfo::CaptureFace tmp;
      if (face.isString()) {
        tmp.url_ = face.asString();
      } else if (face.isObject()) {
        if (face.isMember(kuri) && face[kuri].isString()) {
          tmp.url_ = face[kuri].asString();
        }
        if (face.isMember(kpibx) && face[kpibx].isArray()) {
          for (auto j = 0; j < face[kpibx].size(); ++j) {
            if (face[kpibx][j].isInt())
              tmp.pic_box_.push_back(face[kpibx][j].asInt());
          }
        }
        if (face.isMember(kfcbx) && face[kfcbx].isArray()) {
          for (auto j = 0; j < face[kfcbx].size(); ++j) {
            if (face[kfcbx][j].isInt())
              tmp.face_box_.push_back(face[kfcbx][j].asInt());
          }
        }
      }
      event.capture_uris_.push_back(tmp);
    }
  }
  if (event_jv.isMember(kage) && event_jv[kage].isInt()) {
    event.age_ = event_jv[kage].asInt();
  }
  if (event_jv.isMember(kgender) && event_jv[kgender].isInt()) {
    event.gender_ = event_jv[kgender].asInt();
  }
  return true;
}

std::string BusinessData::GetNextUrl(void) { return event_.GetNextUrl(); }

bool BusinessData::HasNextUrl(void) { return event_.HasNextUrl(); }

int BusinessData::getUrlCount(void) { return event_.capture_uris_.size(); }

void BusinessData::RetreatUrl() { event_.RetreatUrl(); }

std::string BusinessData::getFaceRectByOrder(unsigned int face_num) {
  std::string face_rect = "";

  if (face_num >= event_.capture_uris_.size()) return face_rect;
  auto &face = event_.capture_uris_[face_num];
  if (!face.face_box_.empty() && !face.pic_box_.empty()) {
    std::stringstream ss;
    ss << face.face_box_[0] - face.pic_box_[0] << ",";
    ss << face.face_box_[1] - face.pic_box_[1] << ",";
    ss << face.face_box_[2] - face.pic_box_[0] << ",";
    ss << face.face_box_[3] - face.pic_box_[1];
    face_rect = ss.str();
  }

  return face_rect;
}

int BusinessData::getFaceCount() { return event_.capture_uris_.size(); }

std::string BusinessData::getPicWaitKey(void) {
  std::string maxurl = "";
  std::string temp;
  for (auto face : event_.capture_uris_) {
    temp = hobotcommon::getsplitstr(face.url_);
    if (maxurl == "") {
      maxurl = temp;
    } else {
      if (temp > maxurl) maxurl = temp;
    }
  }
  return maxurl;
}

int64_t BusinessData::getTrackID() {
  return event_.track_id_;
}

int BusinessData::getReplay() {
  return event_.replay_;
}


bool BusinessData::makeKafkaRequest(std::shared_ptr<Session> &session,
                                  std::list<KafkaOutputInfo> &ouput_info_list) {
  Json::Value out_jv;
  Json::Value data_jv;

  int msg_type = session->pHandler_->KafkaData_->event_.msg_type_;
  int gender   = (session->track_result_.gender == -1)?event_.gender_:session->track_result_.gender;
  int age      = (session->track_result_.age == -1)?event_.age_:session->track_result_.age;

  data_jv["app_id"]      = ipc_.app_id_;
  data_jv["space_id"]    = ipc_.space_id_;
  data_jv["device_sn"]   = ipc_.device_id_;
  data_jv["track_id"]    = event_.track_id_;
  data_jv["in_out_type"] = event_.event_type_;
  data_jv["event_time"]  = event_.event_time_;
  data_jv["msg_type"]    = event_.msg_type_;
  data_jv["data_type"]   = session->pHandler_->KafkaData_->plugin_.datatype;

  if (msg_type == 1) {
    data_jv["age"]        = age;
    data_jv["manual_age"] = -1;
    data_jv["gender"]     = gender;
  }
  else if (msg_type == 2) {
    data_jv["face_id"]      = session->person_result_.id;
    data_jv["faceset_id"]   = session->person_result_.set_name;
    data_jv["faceset_type"] = session->person_result_.set_type;
    for (auto &elem : session->person_result_.person_attributes_) {
      if (elem.type_ != "") {
        if (elem.type_ == "gender" || elem.type_ == "manual_age"
            || elem.type_ == "age") {
          data_jv[elem.type_] = atoi(elem.value_.c_str());
        }
        else {
           data_jv[elem.type_] = elem.value_;
        }
      }
    }

    data_jv["score"] =  session->person_result_.similar;

    if (session->pHandler_->KafkaData_->getUrlCount() < 1) {
      LOG_ERROR(
          "[BusinessData::makeKafkaRequest]uid:{} session_id:{} no url ",
          session->uid_, session->session_id_);
      return true;
    }


    for (auto &captureface : event_.capture_uris_) {
      time_t seconds = (time_t)captureface.gen_url_expire_;
      tm  t_tm;
      gmtime_r(&seconds, &t_tm);
      char buf[32] = {0};
      strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &t_tm);
      std::string expire_time(buf);
      Json::Value tmp_jv;
      tmp_jv["expire"] = expire_time;
      tmp_jv["url"] = captureface.gen_url_;
      data_jv["capture_url"] = tmp_jv;
      break;
    }

    {
      time_t seconds = (time_t)session->person_result_.gen_url_expire_;
      tm  t_tm;
      gmtime_r(&seconds, &t_tm);
      char buf[32] = {0};
      strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &t_tm);
      std::string expire_time(buf);
      Json::Value tmp_jv;
      if (session->person_result_.id.substr(0,5) == "noreg") {
        tmp_jv["url"] = "";
        tmp_jv["expire"] = "";
      }
      else {
        tmp_jv["url"] = session->person_result_.gen_url_;
        tmp_jv["expire"] = expire_time;
      }
      
      data_jv["match_url"] = tmp_jv;
    }
  }

  out_jv["data"] = data_jv;
  out_jv["source"] = "/" + ipc_.app_id_ + "/" + ipc_.space_id_ + "/" + ipc_.device_id_;

  if (session->pHandler_->KafkaData_->plugin_.enable) {
    KafkaOutputInfo tmp;
    tmp.data = out_jv.toStyledString();
    for (auto elem : session->pHandler_->KafkaData_->plugin_.topiclist) {
      tmp.topic = elem;
      ouput_info_list.push_back(tmp);
      LOG_INFO(
          "[BusinessData::makeKafkaRequest]uid:{} session_id:{} plugin kafka topic:{} output:{}",
          session->uid_, session->session_id_, tmp.topic, tmp.data);
    }
  }

  KafkaOutputInfo tmp;
  tmp.data = out_jv.toStyledString();
  CONF_PARSER()->getValue(XService::kBusinessCaptureSection, XService::kTopic,
                            tmp.topic);
  ouput_info_list.push_back(tmp);

  LOG_INFO(
        "[BusinessData::makeKafkaRequest]uid:{} session_id:{} default kafka "
        "topic:{} output:{}",
        session->uid_, session->session_id_, tmp.topic, tmp.data);

  return true;
}

bool BusinessData::GetGenerateUrlData(std::shared_ptr<Session>& session,
                                      Json::Value& out_jv) {
  for (auto &captureface : event_.capture_uris_) {
    LOG_INFO(
        "[GenerateUrlModule::MakeRequest]uid:{} session_id:{} url:{}",
         session->uid_, session->session_id_, captureface.url_);
    Json::Value tmp_jv;
    auto uri = captureface.url_;
    auto endpoint = uri.find('/', 0);
    if (endpoint != std::string::npos) {
      tmp_jv[kEndPoint] = uri.substr(0, endpoint);
    } else {
      LOG_ERROR(
          "[GenerateUrlModule::MakeRequest]uid:{} session_id:{} no endpoint",
           session->uid_, session->session_id_);
      return false;
    }
    auto bucket = uri.find('/', endpoint + 1);
    if (bucket != std::string::npos) {
      tmp_jv[kBucket] = uri.substr(
          endpoint + 1, bucket - endpoint - 1);
    } else {
      LOG_ERROR("[GenerateUrlModule::MakeRequest]uid:{} session_id:{} no bucket",
                 session->uid_, session->session_id_);
      return false;
    }
    
    if (bucket + 1 < uri.size()) {
      tmp_jv[kKey] = uri.substr(bucket + 1);
    } else {
      LOG_ERROR("[GenerateUrlModule::MakeRequest]uid:{} session_id:{} no key",
                 session->uid_, session->session_id_);
      return false;
    }
    out_jv["list"].append(tmp_jv);
  }
  return true;
}

bool BusinessData::GetGenUrl(std::shared_ptr<Session>&session,
                                   const Json::Value& body_jv) {

  int i = 0;
  for (auto &captureface : event_.capture_uris_) {
    if (!(body_jv["resultlist"][i].isMember("errorcode") && body_jv["resultlist"][i]["errorcode"].isInt())) {
      return false;
    }
    if (!(body_jv["resultlist"][i].isMember("url") && body_jv["resultlist"][i]["url"].isString())) {
      return false;
    }
    if (!(body_jv["resultlist"][i].isMember("expiretime") && body_jv["resultlist"][i]["expiretime"].isInt())) {
      return false;
    }
    if (body_jv["resultlist"][i]["errorcode"] == 1) {
      captureface.gen_url_ = body_jv["resultlist"][i]["url"].asString();
      captureface.gen_url_expire_ = body_jv["resultlist"][i]["expiretime"].asInt();
    }
    i++;
  }
  if (session->person_result_.url != "") {
    if (!(body_jv["resultlist"][i].isMember("errorcode") && body_jv["resultlist"][i]["errorcode"].isInt())) {
      return false;
    }
    if (!(body_jv["resultlist"][i].isMember("url") && body_jv["resultlist"][i]["url"].isString())) {
      return false;
    }
    if (!(body_jv["resultlist"][i].isMember("expiretime") && body_jv["resultlist"][i]["expiretime"].isInt())) {
      return false;
    }
    if (body_jv["resultlist"][i]["errorcode"] == 1) {
      session->person_result_.gen_url_ = body_jv["resultlist"][i]["url"].asString();
      session->person_result_.gen_url_expire_ = body_jv["resultlist"][i]["expiretime"].asInt();
    }
  
  }
  return true;
}

bool BusinessData::GetSaveMongoData(std::shared_ptr<Session> session) {
  session->saveMongoData_.event_type_ = event_.event_type_;
  session->saveMongoData_.msg_type_ = event_.msg_type_;
  session->saveMongoData_.event_time_ = event_.event_time_;
  session->saveMongoData_.track_id_ = event_.track_id_;
  for (auto &captureface : event_.capture_uris_) {
    session->saveMongoData_.urls.push_back(captureface.url_);
  }
  return true;
}

