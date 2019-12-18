/*
* Copyright 2019 <Copyright hobot>
* @brief Configure manager module, provide a singleton
* to manager configuration
* @author mengmeng.zhi
* @date 25/Dec/2018
*/

#ifndef XBUSINESS_CONF_MANAGER_
#define XBUSINESS_CONF_MANAGER_

#include <string>
#include <mutex>
#include <memory>

#include "configparser/config_parser.h"

namespace XService {
class ConfMgr {
 private:  // Using singleton
  ConfMgr() {}

  ~ConfMgr() {}

 public:
  bool init(const std::string& conf_file);

  /*Get the ini parser*/
  static hobotconfigparser::IniFile* getParser() { return pIniParser_; }

  /*singleton for ConfMgr instance*/
  static ConfMgr* getInstance() {
    // double check locking
    if (NULL == pInstance_) {
      std::lock_guard<std::mutex> lck(singl_mutex_);
      if (NULL == pInstance_) {
        pInstance_ = new (std::nothrow) ConfMgr();
      }
    }
    return pInstance_;
  }

 private:
  std::string conf_file_;

  static std::mutex singl_mutex_;  // mutex for singleton
  static ConfMgr* pInstance_;

  /*It was a basic ini parser from hobot common module*/
  static hobotconfigparser::IniFile* pIniParser_;
};

};  // namespace XService

#define CONMGR() ConfMgr::getInstance()
#define CONF_PARSER() ConfMgr::getParser()

#endif