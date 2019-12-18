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
#include "workflow/module/module_get_plugin.h"
#include "conf/conf_manager.h"
#include "json/json.h"
#include "common/com_func.h"
#include <logmodule/hobotlog.h>
#include "common/mongo/mongo_client_pool.h"

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

GetPluginModule::GetPluginModule() {
  module_name_ = "GetPlugin";
  module_type_ = kGetPlugin;
};

GetPluginModule::~GetPluginModule() {}

bool GetPluginModule::Initialize() {
  return true;
}

void GetPluginModule::Process(std::shared_ptr<Session> session) {
  session->DoRecord(module_name_ + "::Process");
  session->timer_->SetBigTimer(module_name_);
  session->timer_->SetSelfBegin(module_name_);

  bool ret = false;
  ret = getInfoByAppId(session);
  
  session->timer_->SetSelfEnd(this->module_name_);
  if (false == ret) {
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

void GetPluginModule::repairProcChain(std::shared_ptr<Session> session) {
  if (session->pHandler_->KafkaData_->plugin_.enable) {
    auto datatype = session->pHandler_->KafkaData_->plugin_.datatype;
    if (datatype == 2 || datatype == 3) {
      session->proc_chain_.push_back(kOutputKafka);
    }
  }
}

bool GetPluginModule::getInfoByAppId(std::shared_ptr<Session> session) {
  try {
    std::string mongo_collection = "adapter_model";
    auto conn = MongoClientPool::getInstance()->pool_->try_acquire();
    if (!conn) {
      LOG_INFO("[GetPluginModule::getInfoByAppId] {} fail to connect mongo",
               session->session_id_);
      return false;
    }
    mongocxx::client &client = **conn;
    mongocxx::database db = client["businessdb"];
    mongocxx::collection coll = db[mongo_collection];
  
    auto filter_msg = bsoncxx::builder::stream::document{};
    filter_msg << "app_id" << session->pHandler_->KafkaData_->ipc_.app_id_;
    auto filter_msg_val = filter_msg << finalize;

    LOG_INFO("filter msg_id:{} sessionid:{} view {}",
             session->pHandler_->KafkaData_->msg_id_, session->session_id_,
             bsoncxx::to_json(filter_msg_val.view()));

    auto result = coll.find_one(filter_msg_val.view());
    if (result) {
      bsoncxx::document::view view = (*result).view();

      LOG_INFO("msg_id:{} sessionid:{} result view: {}",
               session->pHandler_->KafkaData_->msg_id_, session->session_id_,
               bsoncxx::to_json(view));
      session->pHandler_->KafkaData_->plugin_.enable = true;
      session->pHandler_->KafkaData_->plugin_.service_adapter  = view["service_adapter"].get_utf8().value.to_string();
      session->pHandler_->KafkaData_->plugin_.app_id  = view["app_id"].get_utf8().value.to_string();

      auto element = view["face_rect_from"];
      if (element) {
        session->pHandler_->KafkaData_->plugin_.face_rect_from  = view["face_rect_from"].get_int32().value;
      }
      element = view["face_attr_from"];
      if (element) {
        session->pHandler_->KafkaData_->plugin_.face_attr_from  = view["face_attr_from"].get_int32().value;
      }
      element = view["replay_push_flag"];
      if (element) {
        session->pHandler_->KafkaData_->plugin_.replay_push_flag  = view["replay_push_flag"].get_int32().value;
      }
      element = view["data_channel"];
      if (element) {
        for (auto elem : view["data_channel"].get_array().value) {
          auto datatype = elem["event_type_id"].get_int32().value;
          if (datatype == session->pHandler_->KafkaData_->plugin_.datatype) {
            for (auto subelem : elem["topic_channel"].get_array().value) {
              LOG_INFO("topic_channel={}",subelem.get_utf8().value.to_string());
              session->pHandler_->KafkaData_->plugin_.topiclist.push_back(subelem.get_utf8().value.to_string());
            }
            repairProcChain(session);
            break;
          }
        }      
      }
      return true;
    }
    else {
      return true;
    }
  }
  catch (mongocxx::exception &e) {
    LOG_ERROR("[GetPluginModule::getInfoByAppId] mongo error {} ",e.what());
    return false;
  }
  catch (...) {
    LOG_ERROR("[GetPluginModule::getInfoByAppId] mongo ambiguous error");
    return false;
  }
  return false;
}

