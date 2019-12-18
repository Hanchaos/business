/*
* Copyright 2019 <Copyright hobot>
* @brief Http module that transact with other service
* @author mengmeng.zhi
* @date 25/Dec/2018
*/

#ifndef HTTP_MODULE_WRAPPER_H
#define HTTP_MODULE_WRAPPER_H

#include <iostream>
#include <sstream>
#include "workflow/module_wrapper.h"
#include <transponder/transponder.h>

namespace XService {
class RequestInfo {
 public:
  std::string method_;
  std::string path_;
  std::string body_;
  std::vector<std::pair<std::string, std::string>> extra_;
};

class ModuleFactory;

class HTTPModuleWrapper : public ModuleWrapper {
  friend class ModuleFactory;

 protected:
  HTTPModuleWrapper();

  virtual ~HTTPModuleWrapper() {}

  virtual bool Initialize() = 0;

  virtual void Process(std::shared_ptr<Session>);

  virtual void Finalize() {}

  virtual void HttpErrorProcess(std::shared_ptr<Session>);

  void SendRequest(std::shared_ptr<Session> session, RequestInfo &request);

  virtual bool MakeRequest(std::shared_ptr<Session> session,
                           RequestInfo &request);

  virtual void ProcResponse(std::shared_ptr<Session> session,
                            std::shared_ptr<HttpClient::Response> response);

 private:
  std::atomic_uint concurrent_requests_;
  std::atomic_uint failed_requests_;

 protected:
  /*Download Module need to make http transaction with image server*/
  std::shared_ptr<boost::asio::io_service> io_service_;
  Transponder trans_;
};
}

#endif  // HTTP_MODULE_WRAPPER_H
