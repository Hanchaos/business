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
#include "handler/passenger_flow_handler.h"
#include "workflow/module/module_kafka_output.h"

using namespace XService;

bool PassengerFlowHandler::makeKafkaRequest(
    std::shared_ptr<Session>& session,
    std::list<KafkaOutputInfo>& ouput_info_list) {

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
  for (auto& captureface : KafkaData_->event_.capture_uris_) {
    out_jv["capture_uris"].append(captureface.url_);
  }
  out_jv["replay"] = KafkaData_->event_.replay_;
  out_jv["id"] = "";
  out_jv["score"] = -1;
  out_jv["match_url"] = "";
  out_jv["set_name"] = "";
  out_jv["age"] = KafkaData_->event_.age_;
  out_jv["gender"] = KafkaData_->event_.gender_;

  KafkaOutputInfo tmp;
  tmp.data = out_jv.toStyledString();
  tmp.topic = KafkaData_->task_.kafka_topic_;
  ouput_info_list.push_back(tmp);
  LOG_INFO(
      "[PassengerFlowHandler::makeKafkaRequest]uid:{} session_id:{} kafka "
      "topic:{} output:{}",
      session->uid_, session->session_id_, tmp.topic, tmp.data);

  return true;
}
