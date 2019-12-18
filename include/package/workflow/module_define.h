/*
* Copyright 2019 <Copyright hobot>
* @brief enclosure the rpc calls
* @author mengmeng.zhi
* @date 25/Dec/2018
*/

#ifndef MODULE_COMMON_DEFINE_H
#define MODULE_COMMON_DEFINE_H

namespace XService {
/*
* @brief Define proc type
*   used to define the process steps
* @author mengmeng.zhi
* @date 25/Dec/2018
*/
enum ProcType {
  kNoneProcess = -1,
  kPreProcess = 0,
  kDownloadImage,
  kOutputKafka,
  kPicWait,
  kBusinessSave,
  kGenerateUrl,
  kTraceSpace,
  kGetPlugin,
  kMaxProcType  // new added type must put before this one
};

};  // namespace XService

#endif