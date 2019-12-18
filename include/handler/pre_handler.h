/*
* Copyright 2019 <Copyright hobot>
* @brief Enclosure the Age predict RPC calls
* @author mengmeng.zhi
* @date 25/Dec/2018
*/

#ifndef PRE_HANDLER_H
#define PRE_HANDLER_H

#include <string>

#include "handler/base_handler.h"
#include "session.h"

namespace XService {
/*
* @brief Search call enclosure
* @author mengmeng.zhi
* @date 25/Dec/2018
*/
class PreHandler : public BaseHandler {
 public:
  explicit PreHandler(std::shared_ptr<BaseData> KafkaData) : BaseHandler() {
    this->name_ = "PreHandler";
    this->KafkaData_ = KafkaData;
  }
  virtual void makeProcessChain(std::list<ProcType>& procList) {
    //先看数据类型有没有定制的chain
    if (KafkaData_->makeProcessChain(procList)) {
      return;
    }
    //默认chain
    procList.push_back(kGetPlugin);
    procList.push_back(kDownloadImage);
    procList.push_back(kTraceSpace);
    procList.push_back(kPicWait);
    procList.push_back(kGenerateUrl);
    procList.push_back(kOutputKafka);
    return;
  }

  /*
  * @brief
  * @return Message ID of a PRC call
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  virtual std::string getMsgID() { return KafkaData_->msg_id_; }

  /*
  * @brief Make HTTP request that call from module process
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  virtual bool makeRequest(std::shared_ptr<Session>&, RequestInfo&, ProcType);
  /*
  * @brief Make download request that call from module process
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  virtual bool makeDownloadRequest(std::shared_ptr<Session>&, RequestInfo&);

  virtual bool makeTraceSpaceRequest(std::shared_ptr<Session>&, RequestInfo&);


  virtual bool makeGenerateUrlRequest(std::shared_ptr<Session>& session, RequestInfo& request);


  /*
  * @brief Process HTTP response that call from module process
  * param[in] Session
  * param[out] Response for HTTP
  * param[in] ProcType  module Type
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  virtual bool ProcResponse(std::shared_ptr<Session>&,
                            std::shared_ptr<HttpClient::Response>&, ProcType);

  /*
  * @brief Process image download response
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  virtual bool ProcImageResponse(std::shared_ptr<Session>&,
                                 std::shared_ptr<HttpClient::Response>&);


  virtual bool ProcTraceSpaceResponse(std::shared_ptr<Session>&,
                                      std::shared_ptr<HttpClient::Response>&);


  virtual bool ProcGenerateUrlResponse(std::shared_ptr<Session>&,
                                std::shared_ptr<HttpClient::Response>&);

  virtual bool makeKafkaRequest(std::shared_ptr<Session>&,
                                std::list<KafkaOutputInfo>&);

  virtual bool getPersonResult(std::shared_ptr<Session> &session, std::string &pic_result);

 private:
  Json::Value total_jv_;
};

};  // namespace faceid

#endif  // PRE_HANDLER_H
