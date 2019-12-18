#include "XBusinessHandler.h"
#include "log_module.h"
#include "json/json.h"
#include "handler/pre_handler.h"
#include "handler/base_handler.h"
#include "session.h"
#include "parse_data_factory.h"

using namespace XService;

CXBusinessHandler::CXBusinessHandler() {}

CXBusinessHandler::~CXBusinessHandler() {}

bool CXBusinessHandler::handle(std::shared_ptr<PM_DATA> data,
                               std::string payload) {
  //根据数据解析工厂解析payload type 生成解析数据对象
  std::shared_ptr<BaseData> pKafkaData =
      ParseDataFactory::ParseData(data, payload);
  if (nullptr == pKafkaData) {
    if (nullptr != data) data->state = NODE_STATE_DONE;
    LOG_ERROR("[CXBusinessHandler::handle] not a valid payload type!");
    return false;
  }

  //解析数据
  bool ret = pKafkaData->parse();
  if (!ret) {
    if (nullptr != data) data->state = NODE_STATE_DONE;
    LOG_ERROR("[CXBusinessHandler::handle] Parse KafkaData failed!");
    return false;
  }

  //构造session开始处理
  std::shared_ptr<BaseHandler> pPreHandler =
      std::make_shared<PreHandler>(pKafkaData);
  auto ptrSession = std::make_shared<Session>(pPreHandler);
  WORKER(Session)->PushMsg(ptrSession);

  return true;
}

bool CXBusinessHandler::handleFromRedis(std::string payload, std::string &pic_result) {
  //根据数据解析工厂解析payload type 生成解析数据对象
  std::shared_ptr<BaseData> pKafkaData =
     ParseDataFactory::ParseData(nullptr, payload);
  if (nullptr == pKafkaData) {
    LOG_ERROR("[CXBusinessHandler::handleFromRedis] not a valid payload type!");
    return false;
  }

  //解析数据
  bool ret = pKafkaData->parse();
  if (!ret) {
    LOG_ERROR("[CXBusinessHandler::handleFromRedis] Parse KafkaData failed!");
    return false;
  }

  //构造session开始处理
  std::shared_ptr<BaseHandler> pPreHandler =
      std::make_shared<PreHandler>(pKafkaData);
  auto ptrSession = std::make_shared<Session>(pPreHandler);

  if (!ptrSession->pHandler_->getPersonResult(ptrSession, pic_result)) {
    LOG_ERROR("[CXBusinessHandler::handleFromRedis] getPersonResult failed");
    return false;
  }
  WORKER(Session)->PushMsg(ptrSession);
 
  return true;
}

