#ifndef BASE_DATA_H
#define BASE_DATA_H

#include <string>
#include "com_xservice.h"
#include "log_module.h"
#include <list>
#include "DataDefine.h"
#include "workflow/module_define.h"


namespace XService {
class Session;
class KafkaOutputInfo;

class IPCInfo {
 public:
  std::string space_id_; //设备空间id
  std::string app_id_;   //租户id
  std::string device_id_;
  std::string uid_;  //租户id
  std::string version_;
  int64_t timestamp_;
};

class EventInfo {
 public:
  class CaptureFace {
   public:
    CaptureFace() : gen_url_expire_(0) {}
    std::vector<int> pic_box_;
    std::vector<int> face_box_;
    std::string url_;
    std::string gen_url_;
    long gen_url_expire_;
  };

  EventInfo()
      : group_id_(""),
        extra_(""),
        event_time_(0),
        send_time_(0),
        line_id_(0),
        event_type_(0),
        msg_type_(0),
        track_id_(0),
        age_(0),
        gender_(0),
        url_index_(0),
        replay_(0) {}

  std::string group_id_;
  std::string extra_;
  int64_t event_time_;
  int64_t send_time_;
  int line_id_;
  int event_type_;
  int msg_type_;
  int64_t track_id_;
  std::vector<EventInfo::CaptureFace> capture_uris_;
  int age_;
  int gender_;
  int replay_;

  bool HasNextUrl() { return url_index_ < capture_uris_.size(); }

  std::string GetNextUrl() {
    return url_index_ < capture_uris_.size() ? capture_uris_[url_index_++].url_
                                             : "";
  }

  void RetreatUrl() {
    if (url_index_ > 0) {
      url_index_--;
    }
  }

 private:
  int url_index_;
};

class CaptureInfo {
 public:
  class Background {
   public:
    Background() : tmst_(0), systm_(0) {}

    long long tmst_;
    long long systm_;
    std::list<int> track_ids_;
    std::string url_;
  };

  class Person {
   public:
    class Face {
     public:
      Face() : tmst_(0), systm_(0), age_(0), gender_(0), extra_(0),
                gen_url_expire_(0) {}

      long long tmst_;
      long long systm_;
      std::vector<int> pic_box_;
      std::vector<int> face_box_;
      std::string url_;
      std::string gen_url_;
      long gen_url_expire_;
      int age_;
      int gender_;  // 1：男性； 2： 女性
      int extra_;   // flag
    };

   public:
    Person() : track_id_(0), face_index_(0) {}

    bool HasNextUrl() {
      if (face_index_ < faces_.size()) {
        return true;
      }
      return false;
    }

    std::string GetNextUrl() {
      if (face_index_ < faces_.size()) {
        std::string tmp = faces_[face_index_].url_;
        face_index_++;
        return tmp;
      }
      return "";
    }

    void RetreatUrl() {
      if (face_index_ > 0) {
        face_index_--;
      }
    }

    //获取当前会话抓拍的时间 add by chenjun 2018-10-11
    long long GetCurCapturetm() { return faces_[0].systm_; }

   public:
    int64_t track_id_;
    std::vector<Face> faces_;

   private:
    int face_index_;
  };

 public:
  CaptureInfo() : type_(0), replay_(0) {}

  int type_;    // 0 none; 1 person; 2 background; 3 person and background
  int replay_;  // 0 normal working 1 replay tools working
  Background background_;
  Person person_;
};

class PluginInfo {
  public:
    PluginInfo(): datatype(0),enable(false),face_rect_from(0),face_attr_from(0),replay_push_flag(0) {}
    bool enable;                           // 是否启动插件
    std::string app_id;                    // app_id
    int datatype;                          // 1:ptype8, 2:ptype32 msgtype0, 3:ptype32 msgtype1;4:ptype32 msgtype2;
    int face_rect_from;                    // 0 从设备上带来; 1 云端自己解析
    int face_attr_from;                    // 0 从设备上带来; 1 云端自己解析
    int replay_push_flag;                  // 回灌时，是否推送，0不推，1推送
    std::string service_adapter;           // 插件名称
    std::list<std::string> topiclist;      // 发送的topic列表
};
class BaseData {
 public:
  explicit BaseData(int payload_type, const Json::Value& total_jv,
                    std::shared_ptr<PM_DATA> data)
      : payload_type_(payload_type),
        total_jv_(total_jv),
        data_(data) {
    if (nullptr == data) {
      from = kSrcFromRedis;
    } else {
      from = kSrcFromKafka;
    }
  }
  virtual bool parse(void) { return false; }

  virtual bool makeProcessChain(std::list<ProcType> &procList) { return false; }

  virtual std::string GetNextUrl(void) { return ""; }
  virtual void RetreatUrl() { return; }

  virtual bool HasNextUrl(void) { return false; }

  virtual int getUrlCount(void) { return 0; }

  virtual std::string getFaceRectByOrder(unsigned int face_num) { return ""; }

  virtual int getFaceAgeByOrder(unsigned int face_num) {return -1;}
  
  virtual int getFaceGenderByOrder(unsigned int face_num) {return -1;}

  virtual int getFaceCount(void) { return 0; }

  virtual int64_t getTrackID() { return 0; }

  virtual int getReplay() {return 0;}

  virtual bool makeKafkaRequest(std::shared_ptr<Session> &session,
                                std::list<KafkaOutputInfo> &ouput_info_list) {
    return false;
  }

  virtual std::string getPicWaitKey(void) { return ""; }

  bool parseIpcInfo(Json::Value &);

  virtual bool GetSaveMongoData(std::shared_ptr<Session>) { return false; }

  virtual bool GetGenerateUrlData(std::shared_ptr<Session>& ,
                                      Json::Value& ) { return false; }

  virtual bool GetGenUrl(std::shared_ptr<Session>& ,
                            const Json::Value& ) { return false; }


 public:
  Json::Value total_jv_;

  std::string from;
  IPCInfo ipc_;
  int payload_type_;
  std::string msg_id_;
  std::shared_ptr<PM_DATA> data_;

  EventInfo event_;
  CaptureInfo capture_;
  PluginInfo  plugin_;
};
}
#endif
