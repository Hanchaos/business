/*
* Copyright 2019 <Copyright hobot>
* @brief Enclosure the CommonCaptureHandler
* @author qiong.ye
* @date 24/01/2019
*/

#ifndef COMMON_CAPTURE_HANDLER_H
#define COMMON_CAPTURE_HANDLER_H

#include <string>

#include "handler/base_handler.h"
#include "handler/handler_include.h"
#include "session.h"
#include "conf/conf_manager.h"
#include "com_xservice.h"

namespace XService {

/*
* @brief Search call enclosure
* @author mengmeng.zhi
* @date 25/Dec/2018
*/
class CommonCaptureHandler : public BaseHandler {
 public:
  CommonCaptureHandler() : BaseHandler() { name_ = "CommonCaptureHandler"; }

  CommonCaptureHandler(std::shared_ptr<BaseData> KafkaData) : BaseHandler() {
    name_ = "CommonCaptureHandler";
    KafkaData_ = KafkaData;
  }

  /*
  * @brief Make the process chain of a rpc call
  * @param[out] std::list<ProcType>
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  virtual bool makeProcessChain(std::list<ProcType>& procList) {
    procList.push_back(kPicWait);
    procList.push_back(kDownloadImage);
    procList.push_back(kTraceSpace);
    return true;
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
  * @brief Make static xid request that call from module process
  * @author qiong.ye
  * @date 24/01/2019
  */
  bool makeSearchFeatureStaticRequest(std::shared_ptr<Session>&, RequestInfo&);

  /*
  * @brief Make dynamic xid request that call from module process
  * @author qiong.ye
  * @date 24/01/2019
  */
  bool makeSearchFeatureRequest(std::shared_ptr<Session>&, RequestInfo&);

  /*
  * @brief Make auto register request that call from module process
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  bool makeRegisterPersonRequest(std::shared_ptr<Session>&, RequestInfo&);

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
  bool ProcSearchFeatureStaticResponse(std::shared_ptr<Session>&,
                                       std::shared_ptr<HttpClient::Response>&);
  /*
  * @brief Process feature extract (XFR) response
  * param[in|out] Session
  * param[in] Response for HTTP
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  bool ProcSearchFeatureResponse(std::shared_ptr<Session>&,
                                 std::shared_ptr<HttpClient::Response>&);

  /*
 * @brief Process feature extract (XFR) response
 * param[in|out] Session
 * param[in] Response for HTTP
 * @author mengmeng.zhi
 * @date 25/Dec/2018
 */
  bool ProcRegisterPersonResponse(std::shared_ptr<Session>&,
                                  std::shared_ptr<HttpClient::Response>&);

  bool isAutoReg(std::string);

  virtual bool getAutoRegSet(std::list<std::string>&);
};

};  // namespace faceid

#endif  // COMMON_CAPTURE_HANDLER_H
