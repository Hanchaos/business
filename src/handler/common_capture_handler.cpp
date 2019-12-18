/*
* Copyright 2019 <Copyright hobot>
* @brief Predict call processor
* @author mengmeng.zhi
* @date 25/Dec/2018
*/
#include <string>

#include "handler/handler_include.h"
#include "package/workflow/http_module_wrapper.h"
#include "message/session.h"
#include "json/json.h"
#include "log/log_module.h"
#include "conf/conf_manager.h"
#include "boost/algorithm/string.hpp"
#include "common/com_func.h"

using namespace XService;

bool CommonCaptureHandler::isAutoReg(std::string set_name) {

  std::string str_autoreg = "autoreg";
  int cut_length = set_name.length() - str_autoreg.length();
  if (cut_length > 0) {
    std::string str_postfix = set_name.substr(cut_length);
    if (str_autoreg == str_postfix) return true;
  }
  return false;
}

bool CommonCaptureHandler::getAutoRegSet(std::list<std::string> &set_list) {

  bool is_has_auto_reg_set = false;
  for (auto &set : KafkaData_->task_.trace_.category_sets_) {
    if (isAutoReg(set)) {
      set_list.push_back(set);
      is_has_auto_reg_set = true;
    }
  }
  return is_has_auto_reg_set;
}
