/*
* Copyright 2019 <Copyright hobot>
* @brief Enclosure the Age predict RPC calls
* @author mengmeng.zhi
* @date 25/Dec/2018
*/

#ifndef BUSINESS_CAPTURE_HANDLER_H
#define BUSINESS_CAPTURE_HANDLER_H

#include <string>

#include "handler/base_handler.h"
#include "handler/handler_include.h"
#include "session.h"
#include "conf/conf_manager.h"
#include "com_xservice.h"

namespace XService {
class KafkaOutputInfo;

/*
* @brief Search call enclosure
* @author mengmeng.zhi
* @date 25/Dec/2018
*/
class BusinessCaptureHandler : public BaseHandler {
 public:
  BusinessCaptureHandler(std::shared_ptr<BaseData> KafkaData) : BaseHandler() {
    name_ = "BusinessCaptureHandler";
    KafkaData_ = KafkaData;
    CONF_PARSER()->getValue(XService::kBusinessCaptureSection, XService::kTopic,
                            KafkaData_->task_.kafka_topic_);
  }
  /*
  * @brief Make the process chain of a rpc call
  * @param[out] std::list<ProcType>
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  virtual void makeProcessChain(std::list<ProcType> &procList) {
    procList.push_back(kSearchFeatureStatic);
    procList.push_back(kSearchFeature);
    procList.push_back(kRegisterPerson);
    procList.push_back(kBusinessSave);
    //procList.push_back(kOutputKafka);
  }

  /*
  * @brief Make HTTP request that call from module process
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  virtual bool makeRequest(std::shared_ptr<Session> &, RequestInfo &, ProcType);
  /*
  * @brief Make download request that call from module process
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  virtual bool ProcResponse(std::shared_ptr<Session> &,
                            std::shared_ptr<HttpClient::Response> &, ProcType);

  virtual bool makeKafkaRequest(std::shared_ptr<Session> &,
                                std::list<KafkaOutputInfo> &);

  virtual bool getAutoRegSet(std::list<std::string> &);

  bool isAutoReg(std::string set_name);

  bool makeSearchFeatureStaticRequest(std::shared_ptr<Session> &session,
                                      RequestInfo &request);

  bool makeSearchFeatureRequest(std::shared_ptr<Session> &session,
                                RequestInfo &request);

  bool makeRegisterPersonRequest(std::shared_ptr<Session> &session,
                                 RequestInfo &request);

  bool ProcSearchFeatureStaticResponse(
      std::shared_ptr<Session> &session,
      std::shared_ptr<HttpClient::Response> &response);

  bool ProcSearchFeatureResponse(
      std::shared_ptr<Session> &session,
      std::shared_ptr<HttpClient::Response> &response);

  bool ProcRegisterPersonResponse(
      std::shared_ptr<Session> &session,
      std::shared_ptr<HttpClient::Response> &response);
};

};  // namespace faceid

#endif  // BUSINESS_CAPTURE_HANDLER_H