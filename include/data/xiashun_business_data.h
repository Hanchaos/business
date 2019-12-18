#ifndef XIASHUN_BUSINESS_DATA_H
#define XIASHUN_BUSINESS_DATA_H

#include "base_data.h"
#include "DataDefine.h"

namespace XService {
class xiashunBusinessData : public BaseData {
 public:
  xiashunBusinessData(int payload_type, const Json::Value& total_jv,
               std::shared_ptr<PM_DATA> data)
      : BaseData(payload_type, total_jv, data) {}
  virtual bool parse(void);

  virtual bool makeProcessChain(std::list<ProcType>& procList);

  bool ParseEvent(Json::Value&);

  virtual bool GetSaveMongoData(std::shared_ptr<Session>);

};
}
#endif
