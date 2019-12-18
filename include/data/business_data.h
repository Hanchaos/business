#ifndef BUSINESS_DATA_H
#define BUSINESS_DATA_H

#include "base_data.h"
#include "DataDefine.h"

namespace XService {
class BusinessData : public BaseData {
 public:
  BusinessData(int payload_type, const Json::Value& total_jv,
               std::shared_ptr<PM_DATA> data)
      : BaseData(payload_type, total_jv, data) {}
  virtual bool parse(void);

  virtual bool makeProcessChain(std::list<ProcType>& procList);

  virtual int getUrlCount(void);

  virtual std::string GetNextUrl(void);

  virtual bool HasNextUrl(void);

  virtual void RetreatUrl();

  virtual std::string getFaceRectByOrder(unsigned int face_num);

  virtual int getFaceCount(void);

  virtual std::string getPicWaitKey(void);

  virtual int64_t getTrackID();

  virtual int getReplay();

  bool ParseEvent(Json::Value&);

  virtual bool makeKafkaRequest(std::shared_ptr<Session> &session,
                                    std::list<KafkaOutputInfo> &ouput_info_list);

  virtual bool GetGenerateUrlData(std::shared_ptr<Session>& ,
                                      Json::Value& );

  virtual bool GetGenUrl(std::shared_ptr<Session>&,
                            const Json::Value& );

  virtual bool GetSaveMongoData(std::shared_ptr<Session>);


};
}
#endif
