/*
* Copyright 2019 <Copyright hobot>
* @brief Operate the doc infomation of a person
* @author mengmeng.zhi
* @date 25/Dec/2018
*/

#ifndef XBUSINESS_DOC_OPERATOR_H_
#define XBUSINESS_DOC_OPERATOR_H_

#include <map>

#include <mongocxx/client.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/instance.hpp>
#include <workflow/module_factory.h>

namespace XService {
class ModuleFactory;

class DocOperModule : public ModuleWrapper {
  FRIEND_FACTORY
 public:
  enum Status {
    VIP = 0,
    MATCHED,
    AUTOREG,
    OTHER
  };

 public:
  DocOperModule();

  virtual ~DocOperModule();

  /*
  * @brief Process entry of a session
  * @param[in|out] sessoin that need to be processed
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  virtual void Process(std::shared_ptr<Session>);

  /*
  * @brief Initialize a module
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  virtual bool Initialize();

  /*
  * @brief Finalize a module
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  virtual void Finalize() {}

 private:
  static mongocxx::instance inst_;
  std::unique_ptr<mongocxx::pool> pool_ = nullptr;
  std::string mongo_url_;
  std::string mongo_dbname_;
  std::string set_name_;
};
}  // namespace XService

#endif  // XBUSINESS_DOC_OPERATOR_H_
