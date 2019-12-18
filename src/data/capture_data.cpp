#include "capture_data.h"
#include "com_xservice.h"
#include <list>
#include "DataDefine.h"
#include <string>
#include "workflow/module_define.h"
#include "json/json.h"
#include "common/com_func.h"
#include "session.h"
#include "workflow/module/module_kafka_output.h"
#include "common/mongo/mongo_client_pool.h"
#include <mongocxx/cursor.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/client.hpp>



using namespace XService;

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::array_context;
using bsoncxx::types::b_binary;
using bsoncxx::binary_sub_type;


bool CaptureData::parse(void) {

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

  //解析抓拍信息
  if (!ParseCapture(payload_jv)) {
    return false;
  }

  plugin_.datatype = 1;
  
  return true;
}

bool CaptureData::ParseCapture(Json::Value &payload_jv) {
  Json::Value capture_jv;
  if (payload_jv.isMember(kcapture) && payload_jv[kcapture].isObject()) {
    capture_jv = payload_jv[kcapture];
  } else {
    return false;
  }
  CaptureInfo &capture = capture_;

  if (capture_jv.isMember(kreplay) && capture_jv[kreplay].isInt()) {
    capture.replay_ = capture_jv[kreplay].asInt();
  }

  if (capture_jv.isMember(kpesn) && capture_jv[kpesn].isObject()) {
    capture.type_ += 1;
    if (capture_jv[kpesn].isMember(ktkid) &&
        capture_jv[kpesn][ktkid].isInt64()) {
      capture.person_.track_id_ = capture_jv[kpesn][ktkid].asInt64();
    }
    if (capture_jv[kpesn].isMember(kface) &&
        capture_jv[kpesn][kface].isArray()) {
      for (auto i = 0; i < capture_jv[kpesn][kface].size(); ++i) {
        auto face = capture_jv[kpesn][kface][i];
        CaptureInfo::Person::Face tmp;
        if (face.isMember(ktmst) && face[ktmst].isInt64()) {
          tmp.tmst_ = face[ktmst].asInt64();
        }
        if (face.isMember(ksystm) && face[ksystm].isInt64()) {
          tmp.systm_ = face[ksystm].asInt64();
        }
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
        if (face.isMember(kage) && face[kage].isInt()) {
          tmp.age_ = hobotcommon::Age(face[kage].asInt());
        }
        if (face.isMember(ksex) && face[ksex].isInt()) {
          tmp.gender_ = face[ksex].asInt();
        }
        capture.person_.faces_.push_back(tmp);
      }
    }
  } else if (capture_jv.isMember(kbkgd) && capture_jv[kbkgd].isObject()) {
    capture.type_ += 2;
    auto background = capture_jv[kbkgd];
    if (background.isMember(ktkid) && background[ktkid].isArray()) {
      for (auto j = 0; j < background[ktkid].size(); ++j) {
        capture.background_.track_ids_.push_back(background[ktkid][j].asUInt());
      }
    }
    if (background.isMember(ktmst) && background[ktmst].isInt64()) {
      capture.background_.tmst_ = background[ktmst].asInt64();
    }
    if (background.isMember(ksystm) && background[ksystm].isInt64()) {
      capture.background_.systm_ = background[ksystm].asInt64();
    }
    if (background.isMember(kuri) && background[kuri].isString()) {
      capture.background_.url_ = background[kuri].asString();
    }
  }
  return true;
}

std::string CaptureData::GetNextUrl(void) {
  return capture_.person_.GetNextUrl();
}

bool CaptureData::HasNextUrl(void) { return capture_.person_.HasNextUrl(); }

int CaptureData::getUrlCount(void) { return capture_.person_.faces_.size(); }

void CaptureData::RetreatUrl() { capture_.person_.RetreatUrl(); }

std::string CaptureData::getFaceRectByOrder(unsigned int face_num) {
  std::string faces_rect = "";

  if (face_num >= capture_.person_.faces_.size()) return faces_rect;
  auto &face = capture_.person_.faces_[face_num];
  if (!face.face_box_.empty() && !face.pic_box_.empty()) {
    std::stringstream ss;
    ss << face.face_box_[0] - face.pic_box_[0] << ",";
    ss << face.face_box_[1] - face.pic_box_[1] << ",";
    ss << face.face_box_[2] - face.pic_box_[0] << ",";
    ss << face.face_box_[3] - face.pic_box_[1];
    faces_rect = ss.str();
  }

  return faces_rect;
}

int CaptureData::getFaceAgeByOrder(unsigned int face_num) {

  if (face_num >= capture_.person_.faces_.size()) return -1;
  auto &face = capture_.person_.faces_[face_num];
  return face.age_;
}

int CaptureData::getFaceGenderByOrder(unsigned int face_num) {

  if (face_num >= capture_.person_.faces_.size()) return -1;
  auto &face = capture_.person_.faces_[face_num];
  return face.gender_;
}


int CaptureData::getFaceCount() { return capture_.person_.faces_.size(); }

std::string CaptureData::getPicWaitKey(void) {
  std::string maxurl = "";
  std::string temp;
  for (auto face : capture_.person_.faces_) {
    temp = hobotcommon::getsplitstr(face.url_);
    if (maxurl == "") {
      maxurl = temp;
    } else {
      if (temp > maxurl) maxurl = temp;
    }
  }
  return maxurl;
}


int64_t CaptureData::getTrackID() {
  return capture_.person_.track_id_;
}

int CaptureData::getReplay() {
  return capture_.replay_;
}


bool CaptureData::GetGenerateUrlData(std::shared_ptr<Session>& session,
                                      Json::Value& out_jv) {
  for (auto &captureface : capture_.person_.faces_) {
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

bool CaptureData::GetGenUrl(std::shared_ptr<Session>&session,
                                   const Json::Value& body_jv) {

  int i = 0;
  for (auto &captureface : capture_.person_.faces_) {
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


bool CaptureData::makeKafkaRequest(std::shared_ptr<Session> &session, std::list<KafkaOutputInfo> &ouput_info_list)
{
  Json::Value out_jv;
  Json::Value data_jv;

  if (SwitchFaceSetIdBySetName(session) == false) {
    LOG_INFO("[CaptureData::makeKafkaRequest] {} fail to switch faceset_id {}",
             session->session_id_, session->person_result_.set_name);
    return -1;
  }

  data_jv["app_id"]      = ipc_.app_id_;
  data_jv["space_id"]    = ipc_.space_id_;
  data_jv["device_sn"]   = ipc_.device_id_;
  Json::Int64 tmstp      = capture_.person_.GetCurCapturetm();
  data_jv["event_time"]  = tmstp;
  if (tmstp < 2000000000){
    data_jv["event_time"] = tmstp * 1000;
  }
  data_jv["track_id"]    = capture_.person_.track_id_;

  data_jv["face_id"]      = session->person_result_.id;
  data_jv["faceset_id"]   = session->person_result_.set_name;
  data_jv["faceset_type"] = session->person_result_.set_type;
  data_jv["data_type"]      = session->pHandler_->KafkaData_->plugin_.datatype;

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
       "[CaptureData::makeKafkaRequest]uid:{} session_id:{} no url ",
       session->uid_, session->session_id_);
   return true;
  }


  for (auto &captureface : capture_.person_.faces_) {
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
    data_jv["capture_time"] = (int64_t)captureface.systm_;
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

  if (session->pHandler_->KafkaData_->plugin_.enable) {
    out_jv["data"] = data_jv;
    out_jv["source"] = "/" + ipc_.app_id_ + "/" + ipc_.space_id_ + "/" + ipc_.device_id_;

    KafkaOutputInfo tmp;
    tmp.data = out_jv.toStyledString();
    for (auto elem : session->pHandler_->KafkaData_->plugin_.topiclist) {
      tmp.topic = elem;
      ouput_info_list.push_back(tmp);
      LOG_INFO(
        "[CaptureData::makeKafkaRequest]uid:{} session_id:{} kafka "
        "topic:{} output:{}",
        session->uid_, session->session_id_, tmp.topic, tmp.data);
    }
  }
  else {
   out_jv["data"] = data_jv;
   out_jv["source"] = "/" + ipc_.app_id_ + "/" + ipc_.space_id_ + "/" + ipc_.device_id_;

   KafkaOutputInfo tmp;
   tmp.data = out_jv.toStyledString();
   CONF_PARSER()->getValue(XService::kCommonCaptureSection, XService::kTopic,
                           tmp.topic);

   ouput_info_list.push_back(tmp);

   LOG_INFO(
       "[CaptureData::makeKafkaRequest]uid:{} session_id:{} kafka "
       "topic:{} output:{}",
       session->uid_, session->session_id_, tmp.topic, tmp.data);
  }
  return true;
}

bool CaptureData::SwitchFaceSetIdBySetName(std::shared_ptr<Session> session) {
  try {
    if (session->person_result_.set_name == "") {
      return true;
    }
    std::string mongo_collection = "faceset";
    std::string mongo_dbname     = "businessdb";
    auto conn = MongoClientPool::getInstance()->pool_->try_acquire();
    if (!conn) {
      LOG_INFO("[CaptureData::SwitchFaceSetIdBySetName] {} fail to connect mongo",
               session->session_id_);
      return false;
    }
    mongocxx::client &client = **conn;
    mongocxx::database db = client[mongo_dbname];
    mongocxx::collection coll = db[mongo_collection];
  
    auto filter_msg = bsoncxx::builder::stream::document{};
    filter_msg << "set_name" << session->person_result_.set_name;
    auto filter_msg_val = filter_msg << finalize;
    LOG_INFO("filter msg2   uid:{} {} view {}",
             session->pHandler_->KafkaData_->msg_id_, session->session_id_,
             bsoncxx::to_json(filter_msg_val.view()));

    auto result = coll.find_one(filter_msg_val.view());
    if (result) {
      bsoncxx::document::view view = (*result).view();
      std::string facesetId = view["_id"].get_oid().value.to_string();
      session->person_result_.set_name = facesetId;
      std::string setType = view["faceset_type"].get_utf8().value.to_string();
      session->person_result_.set_type = setType;
      return true;
    }
  }
  catch (...) {
    LOG_ERROR("[CaptureData::QueryFaceSetIdBySetName] mongo error");
    return false;
  }
  return false;
}




