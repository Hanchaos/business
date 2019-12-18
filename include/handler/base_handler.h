/*
* Copyright 2019 <Copyright hobot>
* @brief enclosure the rpc calls
* @author mengmeng.zhi
* @date 25/Dec/2018
*/

#ifndef BASE_HANDLER_H
#define BASE_HANDLER_H

#include <string>
#include <memory>

#include <mongocxx/cursor.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/client.hpp>
#include <workflow/worker.hpp>
#include "package/transponder/transponder.h"
#include "json/json.h"
#include "workflow/module_define.h"
#include "DataDefine.h"
#include "base_data.h"
#include "DataDefine.h"

namespace XService {
class RequestInfo;
class Session;

/*
* @brief RPC call base
* @author mengmeng.zhi
* @date 25/Dec/2018
*/
class BaseHandler {
 public:
  /*
  * @brief Take in the "service" instance (in this case representing an
  * asynchronous
  *       server) and the completion queue "cq" used for asynchronous
  * communication
  *       with the gRPC runtime.
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  BaseHandler() {}
  /*
  * @brief Task dispath
  * @author qiong.ye
  * @date 25/01/2019
  */
  virtual bool TaskRouting(std::shared_ptr<Session> session) { return false; }

  /*
  * @brief It should be called right after the Call was constructed
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  void proceed() {}

 public:
  /*
  * @brief parse data from kafka
  * @author qiong.ye
  * @date 24/01/2019
  */
  virtual bool ParseKafkaData(std::string payload) { return false; }
  /*
  * @brief parse xtrace payload
  * @author qiong.ye
  * @date 24/01/2019
  */
  virtual bool ParseCapture(Json::Value &payload_jv) { return false; }
  /*
  * @brief parse xbusiness payload
  * @author qiong.ye
  * @date 24/01/2019
  */
  virtual bool ParseEvent(Json::Value &payload_jv) { return false; }

 public:
  /*
  * @brief Make the process chain of a rpc call
  * @param[out] std::list<ProcType>
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  virtual void makeProcessChain(std::list<ProcType> &procList) = 0;

  /*
  * @brief Get the message id that from the rpc message
  * @return Message ID of a PRC call
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  virtual std::string getMsgID() { return KafkaData_->msg_id_; }

  /*
  * @brief Make HTTP request that call from module process
  * param[in] Session
  * param[out] RequestInfo for HTTP
  * param[in] ProcType  module Type
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  virtual bool makeRequest(std::shared_ptr<Session> &, RequestInfo &,
                           ProcType) {
    return false;
  }

  /*
  * @brief Make HTTP request that call from module process
  * param[in] Session
  * param[out] Response for HTTP
  * param[in] ProcType  module Type
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  virtual bool ProcResponse(std::shared_ptr<Session> &,
                            std::shared_ptr<HttpClient::Response> &, ProcType) {
    return false;
  }

  virtual bool makeKafkaRequest(std::shared_ptr<Session> &,
                                std::list<KafkaOutputInfo> &) {
    return false;
  }

  virtual bool getAutoRegSet(std::list<std::string> &set_list) { return false; }

  virtual bool getPersonResult(std::shared_ptr<Session> &session, std::string &pic_result) {
    return false;
  }


 public:
  std::shared_ptr<BaseData> KafkaData_;

 protected:
  /*The name of the current Call. Eg, PassengerFlowHandler, SearchCall*/
  std::string name_;

};  // class BaseHandler

};  // namespace faceid

#endif  // BASE_HANDLER_H
