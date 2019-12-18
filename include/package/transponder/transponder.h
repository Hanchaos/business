//
// Created by jianbo on 7/27/18.
//

#ifndef XBUSINESS_TRANSPONDER_H
#define XBUSINESS_TRANSPONDER_H

#include "frame/client_http.h"

namespace XService {

using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;

class Transponder {
 public:
  bool Init(std::shared_ptr<boost::asio::io_service>, const std::string &);

  // async
  void Request(
      const std::string &method, const std::string &path,
      const SimpleWeb::CaseInsensitiveMultimap &headers, std::istream &content,
      std::function<void(std::shared_ptr<HttpClient::Response>,
                         const SimpleWeb::error_code &)> request_callback);

  // sync
  std::shared_ptr<HttpClient::Response> Request(
      const std::string &method, const std::string &path,
      const SimpleWeb::CaseInsensitiveMultimap &headers, std::istream &content);

 public:
  std::shared_ptr<boost::asio::io_service> io_service_;
  std::shared_ptr<HttpClient> client_;
};
}
#endif  // XBUSINESS_TRANSPONDER_H
