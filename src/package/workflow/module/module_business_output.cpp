/*
* Copyright 2019 <Copyright hobot>
* @brief Operate the doc infomation of a person
* @author mengmeng.zhi
* @date 25/Dec/2018
*/
#include <mongocxx/cursor.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/client.hpp>
#include <boost/algorithm/string.hpp>
#include <time.h>
#include "tracktimer/tracktimer.h"

#include "log/log_module.h"
#include "common/com_func.h"
#include "workflow/module/module_business_output.h"
#include "conf/conf_manager.h"
#include "json/json.h"
#include "common/com_func.h"
#include <logmodule/hobotlog.h>
#include "common/mongo/mongo_client_pool.h"

using namespace XService;

static const std::string kSetName = "set_name";
static const std::string kMongoBusinessSection = "mongo_business";
static const std::string kMongoUrl = "mongo_url";
static const std::string kMongoDbName = "mongo_dbname";

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::array_context;
using bsoncxx::types::b_binary;
using bsoncxx::binary_sub_type;

BusinessSave::BusinessSave() {
  module_name_ = "BusinessSave";
  module_type_ = kBusinessSave;
};

BusinessSave::~BusinessSave() {}

bool BusinessSave::Initialize() {
  return true;
}

void BusinessSave::Process(std::shared_ptr<Session> session) {
  session->DoRecord(module_name_ + "::Process");
  session->timer_->SetBigTimer(module_name_);
  session->timer_->SetSelfBegin(module_name_);
  
  int ret = 0;
  ret = SaveBusinessMongo(session);
  session->timer_->SetSelfEnd(this->module_name_);

  if (0 != ret) {
    session->Final(ErrCode::kMongoFailed);
    return;
  }
  if (!session->HasNextProcessor()) {
    session->Final(ErrCode::kSuccess);

    return;
  }
  WORKER(Session)->PushMsg(session);
  return;
}

int64_t bson2Int64(const bsoncxx::document::element &el) {
  if (el) {
    if (el.type() == bsoncxx::type::k_int64) {
      return el.get_int64().value;
    } else if (el.type() == bsoncxx::type::k_int32) {
      return el.get_int32().value;
    }
  }
  return 0;
}

int BusinessSave::SaveBusinessMongo(std::shared_ptr<Session> session) {
  try {
    if (!SwitchFaceSetIdBySetName(session)) {
      LOG_INFO("[BusinessSave::SaveBusinessMongo] {} fail to switch faceset_id {}",
               session->session_id_, session->person_result_.set_name);
      return -1;
    }
    std::string mongo_collection = ""+ session->pHandler_->KafkaData_->ipc_.app_id_ + "_user_event";

    auto conn = MongoClientPool::getInstance()->pool_->try_acquire();
    if (!conn) {
      LOG_INFO("[BusinessSave::SaveBusinessMongo] {} fail to connect mongo",
               session->session_id_);
      return -1;
    }
    mongocxx::client &client = **conn;

    std::string mongo_dbname     = "businessdb";
    mongocxx::database db = client[mongo_dbname];
    mongocxx::collection coll = db[mongo_collection];
    
    int gender = (session->track_result_.gender == -1)?session->pHandler_->KafkaData_->event_.gender_:session->track_result_.gender;
    int age = (session->track_result_.age == 
    -1)?session->pHandler_->KafkaData_->event_.age_:session->track_result_.age;

    if (!session->pHandler_->KafkaData_->GetSaveMongoData(session)) {
      LOG_INFO("[BusinessSave::SaveBusinessMongo] {} fail to get data ",
               session->session_id_);
      return -1;
    }
    int msg_type = session->saveMongoData_.msg_type_;

    time_t seconds = time(NULL);
    tm  t_tm;
    gmtime_r(&seconds, &t_tm);
    char buf[32] = {0};
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &t_tm);
    std::string current_time(buf);

    mongocxx::options::update opt;
    opt.upsert(true);
    auto filer_builder = bsoncxx::builder::stream::document{};
    filer_builder << "device_sn" << session->pHandler_->KafkaData_->ipc_.device_id_ 
               << "track_id" << session->saveMongoData_.track_id_
               << "app_id" << session->pHandler_->KafkaData_->ipc_.app_id_
               << "space_id" << session->pHandler_->KafkaData_->ipc_.space_id_
               << "in_out_type" << session->saveMongoData_.event_type_ ;
    auto filer_builder_val = filer_builder << finalize;
    LOG_INFO("filer_builder_val : uid:{} {} view {}",
             session->pHandler_->KafkaData_->msg_id_, session->session_id_,
             bsoncxx::to_json(filer_builder_val.view()));

    if (msg_type == 1) {
      auto filter_msg = bsoncxx::builder::stream::document{};
      filter_msg << "device_sn" << session->pHandler_->KafkaData_->ipc_.device_id_ 
                     << "track_id" << session->saveMongoData_.track_id_
                     << "app_id" << session->pHandler_->KafkaData_->ipc_.app_id_
                     << "space_id" << session->pHandler_->KafkaData_->ipc_.space_id_
                     << "in_out_type" << session->saveMongoData_.event_type_
                     << "msg_type" << 2;

      auto filter_msg_val = filter_msg << finalize;
      LOG_INFO("filter msg2   uid:{} {} view {}",
               session->pHandler_->KafkaData_->msg_id_, session->session_id_,
               bsoncxx::to_json(filter_msg_val.view()));

      auto result = coll.find_one(filter_msg_val.view());
      if (result) {
        LOG_INFO("mongo has inserted big msgtype!  uid:{} {} view {}",
                 session->pHandler_->KafkaData_->msg_id_, session->session_id_,
                 bsoncxx::to_json(filter_msg_val.view()));
        return 0;
      }
    }

    auto builder = bsoncxx::builder::stream::document{};
    if (msg_type == 2) {
      builder
          <<"$set" << open_document
          << "app_id" <<  session->pHandler_->KafkaData_->ipc_.app_id_
          << "space_id" << session->pHandler_->KafkaData_->ipc_.space_id_
          << "device_sn" << session->pHandler_->KafkaData_->ipc_.device_id_ 
          << "track_id" << session->saveMongoData_.track_id_
          << "face_id" << session->person_result_.id 
          << "faceset_id" << session->person_result_.set_name
          << "faceset_type" << session->person_result_.set_type
          << "event_time" << session->saveMongoData_.event_time_
          << "msg_type" << msg_type
          << "capture_url" << open_array << [&](array_context<> arr) {
            for (auto &captureurl : session->saveMongoData_.urls) {
              arr << captureurl;
            }
          } << close_array
          << "match_url" << session->person_result_.url
          << "score" << session->person_result_.similar
          << "created_time" << current_time
          << "updated_time" << current_time
          << "in_out_type" << session->saveMongoData_.event_type_;
          
          for (auto &elem : session->person_result_.person_attributes_) {
            if (elem.type_ != "") {
              if (elem.type_ == "gender" || elem.type_ == "manual_age"
                  || elem.type_ == "age") {
                builder << elem.type_ << atoi(elem.value_.c_str());
              }
              else {
                builder << elem.type_ << elem.value_;
              }
            }
          }          
    }
    else if (msg_type == 1) {
      builder
          <<"$set" << open_document
          << "app_id" <<  session->pHandler_->KafkaData_->ipc_.app_id_
          << "space_id" << session->pHandler_->KafkaData_->ipc_.space_id_
          << "device_sn" << session->pHandler_->KafkaData_->ipc_.device_id_ 
          << "track_id" << session->saveMongoData_.track_id_
          << "age" << age
          << "gender" << gender
          << "in_out_type" << session->saveMongoData_.event_type_
          << "event_time" << session->saveMongoData_.event_time_
          << "msg_type" << msg_type
          << "created_time" << current_time
          << "updated_time" << current_time;
    }
    else {
      builder
          <<"$set" << open_document
          << "app_id" <<  session->pHandler_->KafkaData_->ipc_.app_id_
          << "space_id" << session->pHandler_->KafkaData_->ipc_.space_id_
          << "device_sn" << session->pHandler_->KafkaData_->ipc_.device_id_ 
          << "track_id" << session->saveMongoData_.track_id_
          << "in_out_type" << session->saveMongoData_.event_type_
          << "event_time" << session->saveMongoData_.event_time_
          << "created_time" << current_time
          << "updated_time" << current_time;
    }
    if (session->pHandler_->KafkaData_->getReplay() == 1){
      auto replay_builder = bsoncxx::builder::stream::document{};
      replay_builder << "device_sn" << session->pHandler_->KafkaData_->ipc_.device_id_ 
                     << "track_id" << session->saveMongoData_.track_id_
                     << "app_id" << session->pHandler_->KafkaData_->ipc_.app_id_
                     << "space_id" << session->pHandler_->KafkaData_->ipc_.space_id_
                     << "in_out_type" << session->saveMongoData_.event_type_ 
                     << "replay" << bsoncxx::builder::stream::open_document << "$ne" << 1 << bsoncxx::builder::stream::close_document;
      auto replay_builder_val = replay_builder << finalize;
      auto result = coll.find_one(replay_builder_val.view());
      if (result) {
        LOG_INFO("[BusinessSave::SaveBusinessMongo]uid:{} {} normal data replay, no need store , view {}",
             session->pHandler_->KafkaData_->msg_id_, session->session_id_, bsoncxx::to_json(replay_builder_val.view())); 
        return 0; }
      builder << "replay" << 1 ;
    }
    builder <<close_document;

    auto doc_val = builder << finalize;
    session->output_ = bsoncxx::to_json(doc_val.view());
    LOG_INFO("[BusinessSave::SaveBusinessMongo]uid:{} {} view {}",
             session->pHandler_->KafkaData_->msg_id_, session->session_id_,
             bsoncxx::to_json(doc_val.view()));

    coll.update_one(filer_builder_val.view(), doc_val.view(), opt);
    LOG_INFO("[BusinessSave::SaveBusinessMongo]uid:{} {} insert success",
             session->pHandler_->KafkaData_->msg_id_, session->session_id_);
  }
  catch (mongocxx::exception &e) {
    LOG_ERROR("[BusinessSave::SaveBusinessMongo] mongo error {}", e.what());
    return -1;
  }
  catch (...) {
    LOG_ERROR("[BusinessSave::SaveBusinessMongo] mongo ambiguous error");
    return -1;
  }
  return 0;
}

bool BusinessSave::SwitchFaceSetIdBySetName(std::shared_ptr<Session> session) {
  try {
    if (session->person_result_.set_name == "") {
      return true;
    }
    std::string mongo_collection = "faceset";
    std::string mongo_dbname     = "businessdb";
    auto conn = MongoClientPool::getInstance()->pool_->try_acquire();
    if (!conn) {
      LOG_INFO("[BusinessSave::SaveBusinessMongo] {} fail to connect mongo",
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
    LOG_ERROR("[BusinessSave::QueryFaceSetIdBySetName] mongo error");
    return false;
  }
  return false;
}


