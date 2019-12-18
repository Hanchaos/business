/*
* Copyright 2019 <Copyright hobot>
* @brief Http module that transact with other service
* @author mengmeng.zhi
* @date 25/Dec/2018
*/

#ifndef MODULE_WRAPPER_H
#define MODULE_WRAPPER_H

#include <iostream>
#include <sstream>
#include <memory>
#include "workflow/module_define.h"
#include "message/session.h"

namespace XService {
class ModuleFactory;
/*
* @brief Interface of all modules,
*        module defines the process step of a rpc call
* @param[in] const std::string& filename
* @author mengmeng.zhi
* @date 25/Dec/2018
*/
class ModuleWrapper {
  friend class ModuleFactory;

 protected:
  ModuleWrapper() {}

  virtual ~ModuleWrapper() {}

 public:
  /*
  * @brief Initialize a module
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  virtual bool Initialize() = 0;

  /*
  * @brief Process entry of a session
  * @param[in|out] sessoin that need to be processed
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  virtual void Process(std::shared_ptr<Session>) = 0;

  /*
  * @brief Finalize a module
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  virtual void Finalize() = 0;

 protected:
  std::string module_name_;

  ProcType module_type_;
};

};  // namespace XService

#endif  // HTTP_MODULE_WRAPPER_H