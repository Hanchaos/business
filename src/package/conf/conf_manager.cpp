/*
* Copyright 2019 <Copyright hobot>
* @brief Configure manager module, provide a singleton
* to manager configuration
* @author mengmeng.zhi
* @date 25/Dec/2018
*/
#include "log/log_module.h"
#include "conf/conf_manager.h"

using namespace XService;

ConfMgr* ConfMgr::pInstance_ = NULL;
std::mutex ConfMgr::singl_mutex_;
hobotconfigparser::IniFile* ConfMgr::pIniParser_ = NULL;

bool ConfMgr::init(const std::string& strConfFile) {
  pIniParser_ = new (std::nothrow) hobotconfigparser::IniFile();
  if (NULL == pIniParser_) {
    return false;
  }
  auto ret = pIniParser_->load(strConfFile);
  if (ret != 0) {
    LOG_ERROR("[Trace::Init] parse cfg {} fail", strConfFile);
    return false;
  }

  return true;
}
