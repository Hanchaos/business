#ifndef SCREENSHOT_DATA_H
#define SCREENSHOT_DATA_H

#include "base_data.h"
#include "DataDefine.h"
#include "session.h"

namespace XService {
class TaskInfo {
 public:
  std::string task_id_;    // 任务ID
  std::string uid_;        // 租户ID
  std::string device_id_;  // 设备ID
  std::string extra_;     // 需要向下透传的其他信息
  int type_;  // 请求类型
};
class ScreenshootData : public BaseData {
 public:
  ScreenshootData(int payload_type, Json::Value total_jv,
                  std::shared_ptr<PM_DATA> data)
      : BaseData(payload_type, total_jv, data) {}
  virtual bool parse(void);

  virtual bool makeProcessChain(std::list<ProcType>& procList);

  bool ParseScrennshot(Json::Value& payload_jv);

  bool parseTasks(Json::Value &);

  virtual bool makeKafkaRequest(std::shared_ptr<Session>& session,
                                std::list<KafkaOutputInfo>& ouput_info_list);

 public:
  std::string payload_;
  std::list<TaskInfo> taskArry_;
};
}
#endif