//
// Created by jianbo on 7/28/18.
//
#include "log/log_module.h"
#include "workflow/module/module_kafka_output.h"
#include "conf/conf_manager.h"
#include "com_xservice.h"

namespace XService {
static void DrMsgCallback(rd_kafka_t *rk, const rd_kafka_message_t *rkmessage,
                          void *opaque) {
  if (rkmessage->err) {
    LOG_TRACE("[KafkaWrapper::Callback] {} delivery failed: {}",
              rkmessage->payload, rd_kafka_err2str(rkmessage->err));
  } else {
    LOG_TRACE("[KafkaWrapper::Callback] {} delivery success",
              rkmessage->payload);
  }
}

OutPutKafkaModule::OutPutKafkaModule() {
  std::string brokers;
  int ret = CONF_PARSER()->getValue(kKafkaSection, kBrokers, brokers);
  if (0 != ret) {
    brokers = "";
  }
  brokers_ = brokers;
  module_name_ = "OutputKafkaModule";
  module_type_ = kOutputKafka;
}

OutPutKafkaModule::~OutPutKafkaModule() {}

bool OutPutKafkaModule::Initialize() {
  char errstr[512];
  rd_kafka_conf_t *conf = rd_kafka_conf_new();
  if (rd_kafka_conf_set(conf, "bootstrap.servers", brokers_.c_str(), errstr,
                        sizeof(errstr)) != RD_KAFKA_CONF_OK) {
    LOG_ERROR("[KafkaWrapper::Initialize] {}", errstr);
    return false;
  }

  rd_kafka_conf_set_dr_msg_cb(conf, DrMsgCallback);

  ptr_rk_ = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
  if (!ptr_rk_) {
    LOG_ERROR("[KafkaWrapper::Initialize] failed to create producer: {}",
              errstr);
    return false;
  }
  return true;
}

void OutPutKafkaModule::Process(std::shared_ptr<Session> session) {

  session->DoRecord("OutPutKafkaModule::Process");
  session->timer_->SetBigTimer(module_name_);
  std::list<KafkaOutputInfo> send_datas;
  if (session->pHandler_->makeKafkaRequest(session, send_datas) == false) {
    LOG_ERROR(
        "[{} OutPutKafkaModule::Process] uid:{} sessionid:{} make kafka "
        "request fail!",
        module_name_, session->uid_, session->session_id_);
    session->Final(ErrCode::kKafkaError);
    return;
  }
  session->timer_->SetSelfBegin(module_name_);
  if (session->pHandler_->KafkaData_->getReplay() == 1 && session->pHandler_->KafkaData_->plugin_.replay_push_flag == 0) {
    LOG_INFO(
        "[{} OutPutKafkaModule::Process] uid:{} sessionid:{} replay data , no push to kafka",
        module_name_, session->uid_, session->session_id_);
  } else {
    for (auto &ouput : send_datas) {
      if (!push_data_to_kafka(ouput.data, ouput.topic, session)) {
        session->Final(ErrCode::kKafkaError);
        return;
      }
    }
  }
  

  session->timer_->SetSelfEnd(module_name_);
  if (!session->HasNextProcessor()) {
    session->Final(ErrCode::kSuccess);
    return;
  }
  WORKER(Session)->PushMsg(session);
  return;
}

void OutPutKafkaModule::Finalize() {
  if (ptr_rk_ != NULL) {
    rd_kafka_flush(ptr_rk_, 10 * 1000 /* wait for max 10 seconds */);
    for (auto &couple : rkt_dict_) {
      auto rkt = (rd_kafka_topic_t *)couple.second;
      rd_kafka_topic_destroy(rkt);
    }
    rd_kafka_destroy(ptr_rk_);
  }
  ptr_rk_ = NULL;
}

void OutPutKafkaModule::ProcError(std::shared_ptr<Session> session) {
  LOG_ERROR("[KafkaWrapper::Process]uid:{} session_id:{} fail enqueue msg",
            session->uid_, session->session_id_);
  session->Final(ErrCode::kKafkaError);
}
bool OutPutKafkaModule::push_data_to_kafka(std::string &data,
                                           std::string &topic,
                                           std::shared_ptr<Session> session) {

  rd_kafka_topic_t *rkt = NULL;
  do {
    std::unique_lock<std::mutex> lck(mtx_);
    auto iter = rkt_dict_.find(topic);
    if (iter != rkt_dict_.end()) {
      rkt = (rd_kafka_topic_t *)iter->second;
      break;
    } else {
      rkt = rd_kafka_topic_new(ptr_rk_, topic.c_str(), NULL);
      if (!rkt) {
        LOG_ERROR("[KafkaWrapper::Process] failed to create topic: {}",
                  rd_kafka_err2str(rd_kafka_last_error()));
      } else {
        rkt_dict_[topic] = rkt;
      }
    }
  } while (0);

  if (NULL == rkt) {
    LOG_ERROR(
        "[OutPutKafkaModule::push_data_to_kafka]uid:{} session_id:{} fail "
        "enqueue msg",
        session->uid_, session->session_id_);
    return false;
  }
  auto retry = 0;
  bool is_success = false;
  while (retry++ < 3) {
    if (rd_kafka_produce(rkt, RD_KAFKA_PARTITION_UA, RD_KAFKA_MSG_F_COPY,
                         (void *)data.c_str(), data.length(), NULL, 0,
                         NULL) == -1) {
      LOG_WARN(
          "[OutPutKafkaModule::push_data_to_kafka]uid:{} session_id:{} failed "
          "to produce to topic {}: {}",
          session->uid_, session->session_id_, rd_kafka_topic_name(rkt),
          rd_kafka_err2str(rd_kafka_last_error()));
      /* Poll to handle delivery reports */
      if (rd_kafka_last_error() == RD_KAFKA_RESP_ERR__QUEUE_FULL) {
        rd_kafka_poll(ptr_rk_, 1000 /*block for max 1000ms*/);
      } else {
        break;
      }
    } else {
      LOG_TRACE(
          "[OutPutKafkaModule::push_data_to_kafka]uid:{} session_id:{} "
          "enqueued msg",
          session->uid_, session->session_id_);
      is_success = true;
      break;
    }
  }
  rd_kafka_poll(ptr_rk_, 0 /*non-blocking*/);
  if (false == is_success) {
    LOG_ERROR(
        "[OutPutKafkaModule::push_data_to_kafka]uid:{} session_id:{} fail "
        "enqueue msg",
        session->uid_, session->session_id_);
    return false;
  }
  return true;
}
}
