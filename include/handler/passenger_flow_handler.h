/*
* Copyright 2019 <Copyright hobot>
* @brief Enclosure the Register RPC calls
* @author mengmeng.zhi
* @date 25/Dec/2018
*/

#ifndef PASSENGER_FLOW_HANDLER_
#define PASSENGER_FLOW_HANDLER_

#include <string>
#include "handler/pre_handler.h"
#include "handler/base_handler.h"
#include "conf/conf_manager.h"
#include "session.h"

namespace XService {
/*
* @brief Register call enclosure
* @author mengmeng.zhi
* @date 25/Dec/2018
*/
class PassengerFlowHandler : public BaseHandler {
 public:
  PassengerFlowHandler() : BaseHandler() { name_ = "PassengerFlowHandler"; }
  PassengerFlowHandler(std::shared_ptr<BaseData> KafkaData) : BaseHandler() {
    name_ = "PassengerFlowHandler";
    KafkaData_ = KafkaData;
    CONF_PARSER()->getValue(XService::kPassengerFlowSection, XService::kTopic,
                            KafkaData_->task_.kafka_topic_);
  }

  /*
  * @brief Make the process chain of a rpc call
  * @param[out] std::list<ProcType>
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  virtual void makeProcessChain(std::list<ProcType>& procList) {
    procList.push_back(kOutputKafka);
  }

  /*
  * @brief
  * @return Message ID of a PRC call
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  virtual std::string getMsgID() { return KafkaData_->msg_id_; }

  virtual bool makeKafkaRequest(std::shared_ptr<Session>&,
                                std::list<KafkaOutputInfo>&);
};
};

#endif  // PASSENGER_FLOW_HANDLER_
