#include "workflow/module/module_picture_waiting.h"
#include "log/log_module.h"
#include "conf/conf_manager.h"
#include "com_xservice.h"
#include "XBusinessHandler.h"

using namespace XService;

PictureWaitModule::PictureWaitModule() {
  module_name_ = "PictureWaitingModule";
  module_type_ = kPicWait;
}
PictureWaitModule::~PictureWaitModule() {}

bool PictureWaitModule::Init(const QRedisParam& params) {
  rediscli_ = CRedisFactory::getInstance()->CreateRedisClient(params);
  if (!rediscli_) return false;
  return true;
}

bool PictureWaitModule::Initialize() {
  QRedisParam param;
  int ret = -1;
  ret = CONF_PARSER()->getValue(kRedisSection, kSentinel, param.sentinel_name);
  if (0 != ret) {
    LOG_ERROR("[RedisModule::Initialize] sentinel_name is null");
    return false;
  }
  ret = CONF_PARSER()->getValue(kRedisSection, kAddrs, param.redis_addrs);
  if (0 != ret) {
    LOG_ERROR("[RedisModule::Initialize] redis_addrs is null");
    return false;
  }
  ret = CONF_PARSER()->getValue(kRedisSection, kPasswd, param.passwd);
  if (0 != ret) {
    LOG_ERROR("[RedisModule::Initialize] passwd is null");
    return false;
  }
  param.io_num = CONF_PARSER()->getIntValue(kRedisSection, kIoNum, ret);
  if (0 != ret) {
    LOG_ERROR("[RedisModule::Initialize] io_num is null");
    return false;
  }

  param.db_index = CONF_PARSER()->getIntValue(kRedisSection, kPicIndex, ret);
  if (0 != ret) {
    LOG_ERROR("[RedisModule::Initialize] db_index is null");
    return false;
  }

  Init(param);
  return true;
}

int PictureWaitModule::handleWithoutKey(std::shared_ptr<Session> &session, std::string &key, 
          std::string &payload, int &ptype) {
  int ret_flag = PictureWaitResult::PASS;
  Json::Value value_json;
  value_json["ptype"] = Json::Value(ptype);

  if (ptype == XService::PayloadType::Business) {
    ret_flag = PictureWaitResult::WAIT;
    value_json["message_list"].append(Json::Value(payload));
  }
  else {
    ret_flag = PictureWaitResult::PASS;
    value_json["trace_result"] = Json::Value(session->trace_space_result_);
  }

  if (!(rediscli_->SetAdvancedKeyValue(key, value_json.toStyledString(),
                                       picturewait_cache_timeout, false))) {
    LOG_ERROR("[PictureWait::handleWithoutKey] set url key:{} exp:{} fail",
              key, picturewait_cache_timeout);
  }
  return ret_flag;
}


int PictureWaitModule::handleWithKey(std::shared_ptr<Session> &session, std::string &key, 
          std::string &vaule, std::string &payload, int &ptype) {

  Json::Reader message_reader;
  int ret_flag = PictureWaitResult::PASS;
  Json::Value data_json;
  if (!message_reader.parse(vaule, data_json)) {
    return PictureWaitResult::PASS;
  }

  if (!(data_json.isMember("ptype") && data_json["ptype"].isInt())) {
    return PictureWaitResult::PASS;
  }

  int vaule_type = data_json["ptype"].asInt();

  switch (vaule_type)
  {
    case XService::PayloadType::Trace:

      if (ptype == XService::PayloadType::Business) {
        if (data_json.isMember("trace_result") && data_json["trace_result"].isString()) {
          std::string trace_result = data_json["trace_result"].asString();
          session->pHandler_->getPersonResult(session, trace_result);
        }
      }
      else if (ptype == XService::PayloadType::Trace) {
      }
      break;
    case XService::PayloadType::Business:

      if (ptype == XService::PayloadType::Business) {
        data_json["message_list"].append(Json::Value(payload));
        if (!(rediscli_->SetAdvancedKeyValue(
                 key, data_json.toStyledString(), picturewait_cache_timeout, false))) {
          LOG_ERROR(
              "[PictureWait::handleWithKey] set url key:{} exp:{} fail",
              key, picturewait_cache_timeout);
        }
        ret_flag = PictureWaitResult::WAIT;
      }
      else if (ptype == XService::PayloadType::Trace) {
        for (int k = 0; k < data_json["message_list"].size(); k++) {
          if (data_json["message_list"][k].isString())
            CXBusinessHandler::getInstance()->handleFromRedis(data_json["message_list"][k].asString(), session->trace_space_result_);
        }
        Json::Value value_json;
        value_json["trace_result"] = Json::Value(session->trace_space_result_);
        value_json["ptype"] = Json::Value(ptype);
        if (!(rediscli_->SetAdvancedKeyValue(
                 key, value_json.toStyledString(), picturewait_cache_timeout, false))) {
          LOG_ERROR(
              "[PictureWait::handleWithKey] set url key:{} exp:{} fail",
              key, picturewait_cache_timeout);
        }
      }
      break;
    default:
      break;
  }
  return ret_flag;
}



int PictureWaitModule::getResultOfWait(std::shared_ptr<Session> session) {
  Json::Reader message_reader;
  int ret_flag = PictureWaitResult::PASS;
  int is_locked = false;
  std::string key;
  std::string lock_key;

  do {
    int ptype = session->pHandler_->KafkaData_->payload_type_;
    std::string payload =
        (char*)(session->pHandler_->KafkaData_->data_->msg->payload());

    key = session->pHandler_->KafkaData_->getPicWaitKey();
    if (key == "") {
      break;
    }
    lock_key = "lock_" + key;

    session->DoRecord("redlock");
    //加分布式锁
    is_locked =
        rediscli_->RedLock(lock_key, session->uid_, redlock_acquire_timeout_,
                           redlock_expire_timeout_);
    if (!is_locked) {
      LOG_ERROR("[PictureWait::getResultOfWait] get lock:{} fail", lock_key);
      break;
    }
    std::string get_data;
    bool ret = rediscli_->GetValueByKey(key, get_data);
    //没有key或值为空  直接set
    if (!ret || get_data == "") {
      ret_flag = handleWithoutKey(session, key, payload, ptype);
      break;
    }
    //有value的情况
    else {
      ret_flag = handleWithKey(session, key, get_data, payload, ptype);
      break;
    }
  } while (0);

  //释放分布式锁
  if (is_locked && !rediscli_->RedUnlock(lock_key, session->uid_)) {
    LOG_ERROR("[PictureWait::getResultOfWait] release lock:{} fail", lock_key);
  }
  session->DoRecord("redunlock");
  return ret_flag;
}

void PictureWaitModule::Process(std::shared_ptr<Session> session) {
  session->timer_->SetBigTimer(module_name_);
  session->DoRecord(this->module_name_ + "::Process");

  /*do something*/
  if (getResultOfWait(session) == PictureWaitResult::WAIT) {
    session->Final(ErrCode::kSuccess);
    return;
  }
  /*do something*/

  // If all the processor has been done, the session will be finished.
  if (!session->HasNextProcessor()) {
    session->Final(ErrCode::kSuccess);
    return;
  }

  WORKER(Session)->PushMsg(session);

  return;
}
