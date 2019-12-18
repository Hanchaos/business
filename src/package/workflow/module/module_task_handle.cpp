#include "workflow/module/module_task_handle.h"
#include "log/log_module.h"
#include "conf/conf_manager.h"
#include "com_xservice.h"

using namespace XService;

TaskHandleModule::TaskHandleModule() {
  module_name_ = "TaskHandleModule";
  module_type_ = kTaskHandle;
}
TaskHandleModule::~TaskHandleModule() {
  if (taskcache_ != nullptr) {
    delete taskcache_;
  }
  if (t_ != nullptr) {
    t_->Expire();
    delete t_;
  }
}

bool TaskHandleModule::Init(const QRedisParam& param, int refresh_interval) {
  rediscli_ = CRedisFactory::getInstance()->CreateRedisClient(param);

  redissub_ = CRedisFactory::getInstance()->CreateRedisSubscriber(
      param, param.sub_key,
      std::bind(&TaskHandleModule::RedisCB, this, std::placeholders::_1));

  if (!rediscli_ || !redissub_) return false;
  taskcache_ = new lrucache::Cache<std::string, std::string, std::mutex>(
      cache_max_size_, cache_elasticity_);
  t_ = new Timer();
  t_->StartTimer(refresh_interval,
                 std::bind(&TaskHandleModule::RefreshTaskCache, this));

  return true;
}

bool TaskHandleModule::Initialize() {
  QRedisParam param;
  int ret = -1;
  ret = CONF_PARSER()->getValue(kRedisSection, kSentinel, param.sentinel_name);
  if (0 != ret) {
    LOG_ERROR("[TaskHandleModule::Initialize] sentinel_name is null");
    return false;
  }
  ret = CONF_PARSER()->getValue(kRedisSection, kAddrs, param.redis_addrs);
  if (0 != ret) {
    LOG_ERROR("[TaskHandleModule::Initialize] redis_addrs is null");
    return false;
  }
  ret = CONF_PARSER()->getValue(kRedisSection, kPasswd, param.passwd);
  if (0 != ret) {
    LOG_ERROR("[TaskHandleModule::Initialize] passwd is null");
    return false;
  }
  param.io_num = CONF_PARSER()->getIntValue(kRedisSection, kIoNum, ret);
  if (0 != ret) {
    LOG_ERROR("[TaskHandleModule::Initialize] io_num is null");
    return false;
  }
  ret = CONF_PARSER()->getValue(kRedisSection, kSubKey, param.sub_key);
  if (0 != ret) {
    LOG_ERROR("[TaskHandleModule::Initialize] sub_key is null");
    return false;
  }
  ret = CONF_PARSER()->getValue(kRedisSection, kConfigKey, param.config_key);
  if (0 != ret) {
    LOG_ERROR("[TaskHandleModule::Initialize] config_key is null");
    return false;
  }
  ret =
      CONF_PARSER()->getValue(kRedisSection, kConfigValue, param.config_value);
  if (0 != ret) {
    LOG_ERROR("[TaskHandleModule::Initialize] config_value is null");
    return false;
  }
  param.db_index = CONF_PARSER()->getIntValue(kRedisSection, kDbIndex, ret);
  if (0 != ret) {
    LOG_ERROR("[TaskHandleModule::Initialize] db_index is null");
    return false;
  }
  int refresh_interval =
      CONF_PARSER()->getIntValue(kRedisSection, kRefreshInterval, ret);
  if (0 != ret) {
    LOG_ERROR("[TaskHandleModule::Initialize] refresh_interval is null");
    return false;
  }
  Init(param, refresh_interval);
  return true;
}

bool TaskHandleModule::getAndParseTask(std::shared_ptr<Session> session) {
  if (!session->pHandler_->KafkaData_->hasTaskData_) {
    std::string data;
    Json::Reader task_reader;

    //从redis中获取任务列表
    bool ret = GetTask(session->pHandler_->KafkaData_->ipc_.device_id_, data);
    if (!ret || data == "") {
      LOG_ERROR("[ PreHandler::doRedis] failed {}",
                session->pHandler_->KafkaData_->ipc_.device_id_);
      return false;
    }
    if (!task_reader.parse(data, session->pHandler_->KafkaData_->total_jv_)) {
      LOG_ERROR("[ PreHandler::doRedis] parse redis value by json failed");
      return false;
    }
    //解析任务数据
    if (!session->pHandler_->KafkaData_->parseTasks(
             session->pHandler_->KafkaData_->total_jv_)) {
      LOG_ERROR("[ PreHandler::doRedis] parse taskjson failed");
      return false;
    }
  }
  return true;
}

void TaskHandleModule::Process(std::shared_ptr<Session> session) {
  session->timer_->SetBigTimer(module_name_);
  session->DoRecord(this->module_name_ + "::Process");
  if (!getAndParseTask(session) || !session->pHandler_->TaskRouting(session)) {
    session->Final(ErrCode::kTaskHandleError);
    return;
  }

  // If all the processor has been done, the session will be finished.
  if (!session->HasNextProcessor()) {
    session->Final(ErrCode::kSuccess);

    return;
  }

  WORKER(Session)->PushMsg(session);

  return;
}

bool TaskHandleModule::GetTask(const std::string& key, std::string& data) {
  if (taskcache_->tryGet(key, data)) {
    return true;
  }
  if (rediscli_->GetValueByKey(key, data)) {
    taskcache_->insert(key, data);
    return true;
  }
  return false;
}

void TaskHandleModule::RedisCB(const SubResult& res) {
  LOG_DEBUG("[ TaskHandleModule::redisCB] trigget event oper:{} key:{}", res.oper_,
            res.key_);
  if (taskcache_->remove(res.key_)) {
    LOG_DEBUG("[ TaskHandleModule::redisCB] remove taskcache key:{}", res.key_);
  }
}

int TaskHandleModule::RefreshTaskCache() {
  taskcache_->clear();
  LOG_DEBUG("[ TaskHandleModule::RefreshTaskCache] refresh lru cache timely");
}
