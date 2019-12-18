#ifndef CAPTURE_DATA_H
#define CAPTURE_DATA_H

#include "base_data.h"
#include "DataDefine.h"

namespace XService {

class CaptureData : public BaseData {
 public:
  CaptureData(int payload_type, const Json::Value& total_jv,
              std::shared_ptr<PM_DATA> data)
      : BaseData(payload_type, total_jv, data) {}
  virtual bool parse(void);

  virtual std::string GetNextUrl(void);

  virtual bool HasNextUrl(void);

  virtual int getUrlCount(void);

  virtual void RetreatUrl();

  virtual std::string getFaceRectByOrder(unsigned int face_num);

  virtual int getFaceCount(void);

  virtual std::string getPicWaitKey(void);

  virtual int getFaceAgeByOrder(unsigned int face_num);
  
  virtual int getFaceGenderByOrder(unsigned int face_num);

  bool ParseCapture(Json::Value &);


  virtual bool GetGenUrl(std::shared_ptr<Session>&session, const Json::Value& body_jv);


  virtual bool GetGenerateUrlData(std::shared_ptr<Session>& session, Json::Value& out_jv);

  virtual bool makeKafkaRequest(std::shared_ptr<Session> &session, std::list<KafkaOutputInfo> &ouput_info_list);


  bool SwitchFaceSetIdBySetName(std::shared_ptr<Session> session);

  virtual int64_t getTrackID();

  virtual int getReplay();

};
}
#endif
