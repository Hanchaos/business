/*
* Copyright 2019 <Copyright hobot>
* @brief Operate the doc infomation of a person
* @author mengmeng.zhi
* @date 25/Dec/2018
*/

#ifndef TRACE_OUTPUT_H
#define TRACE_OUTPUT_H

#include <map>

#include <mongocxx/client.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/instance.hpp>
#include <workflow/module_factory.h>

#include <xsearch/common/xsearch_data.h>
#include "xsearch/xsearch.h"

namespace XService {

class ModuleFactory;
class Session;
class MatchPersonInfo;

class TraceSave : public ModuleWrapper {
  FRIEND_FACTORY
 public:
  enum Status {
    VIP = 0,
    MATCHED,
    AUTOREG,
    OTHER
  };

 public:
  TraceSave();

  virtual ~TraceSave();

  /*
  * @brief Process entry of a session
  * @param[in|out] sessoin that need to be processed
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  virtual void Process(std::shared_ptr<Session>);

  virtual bool Initialize();

  /*
  * @brief Finalize a module
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  virtual void Finalize() {}

  int GetCenterID(std::shared_ptr<Session> session);

  int SaveTraceMongo(std::shared_ptr<Session> session, int d_cluster_id);

  bool CheckInMongo(std::shared_ptr<Session> session);

  bool IsRealMatch(std::shared_ptr<Session> session);

  int FillMatchInfo(std::shared_ptr<Session> session,
                    MatchPersonInfo &matched_person);

 private:
  static mongocxx::instance inst_;
  std::unique_ptr<mongocxx::pool> pool_ = nullptr;
  std::string mongo_url_;
  std::string mongo_dbname_;
  std::string set_name_;
  std::string model_version_;
  std::string house_name_;
  std::map<std::string, bool> index_add_flag_;

  int XSearchInit(std::string set_name, std::string model_version,
                  std::string house_name);
};
}  // namespace XService

#endif  // TRACE_OUTPUT_H
