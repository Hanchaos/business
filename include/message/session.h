/*
* Copyright 2019 <Copyright hobot>
* @brief Session that reflect to service side
* @author mengmeng.zhi
* @date 25/Dec/2018
*/
#ifndef COMMON_SESSION_H
#define COMMON_SESSION_H

#include <list>
#include <vector>
#include <future>
#include "xperf.h"

#include "handler/base_handler.h"

namespace XService {

namespace ErrCode {
const int kSuccess = 0;
const int kError = 1000;
const int kProcTerminate = 1001;
const int kFeatureExtractError = 1002;
const int kFeatureSearchError = 1003;
const int kKafkaError = 1004;
const int kMsgTypeError = 1005;
const int kMsgFormatError = 1006;
const int kDownloadError = 1007;
const int kXflowError = 1008;
const int kHttpConnError = 1009;
const int kPersonRegisteError = 1010;
const int kTimeout = 1100;
const int kMongoFailed = 1101;
const int kInvalidInput = 1102;
const int kTaskHandleError = 1103;
const int kIDExisted = 200500;
// const int kFullQueue = 1101;
};

namespace ErrDesc {
const std::string ERR_DESC_Succ = "Success";
const std::string ERR_DESC_Unknown = "UnKnown Error";
const std::string ERR_DESC_ProcTerminate = "Process terminate by exception";
const std::string ERR_DESC_FeatureExt = "Failed to extract feature of images";
const std::string ERR_DESC_FeatureSearch = "Failed to search feature";
const std::string ERR_DESC_Download = "Failed to download the image";
const std::string ERR_DESC_HttpConn = "Failed to make http connections";
const std::string ERR_DESC_Register = "Failed to register person";
const std::string ERR_DESC_Timeout = "TimeOut";
}

class MatchPersonInfo {
 public:
  class Attribute {
   public:
    std::string type;
    std::string value;
    std::string confidence;
    std::string mtime;
    std::string is_manual;
  };

 public:
  std::string id;
  double distance;
  double similar;
  std::string set_name;
  std::string model_version;
  std::list<std::string> urls;
  std::list<Attribute> attrs;
  std::string atime;
  std::string ctime;
};

class PersonResultAttribute {
public:
  PersonResultAttribute() : confidence_(0),mtime_(0),is_manual_(false) {}
  std::string type_;
  std::string value_;
  int         confidence_;
  int         mtime_;
  bool        is_manual_;
};

class PersonResult {
public:
  PersonResult() : gen_url_expire_(0),is_auto_reg(0),type(0),atime(0) {}
  std::string id;
  int type;
  double similar;
  std::string set_name;
  std::string set_type;
  std::string url;
  int is_auto_reg;
  std::string gen_url_;
  long gen_url_expire_;
  long atime;
  std::list<PersonResultAttribute> person_attributes_;
};

class TrackResult {
public:
  TrackResult() {
    age = -1;
    gender = -1;
    score = 0;
  }

  int age;
  int gender;
  double score;
};


class ImageInfo {
 public:
  std::string url_;
  std::string img_;
};

class XFrResult {
 public:
  XFrResult() {
    age = -1;
    gender = -1;
    autoreg_score = 0;
    can_autoreg = false;
  }

  class Attribute {
   public:
    std::string type;
    std::string value;
    std::string confidence;
    std::string mtime;
    std::string is_manual;
  };

 public:
  int age;
  int gender;
  float autoreg_score;
  int can_autoreg;
  std::string can_reg_img;
  std::string img_scores;
  std::vector<Attribute> attrs;
};

class SaveMongoData {
 public :
  int event_type_;
  int msg_type_;
  int64_t event_time_;
  int64_t track_id_;
  std::vector<std::string> urls;
};

// MultiThread Unsafe!!!
class Session : public PerfTPInfo,
                public std::enable_shared_from_this<Session> {
 public:
  explicit Session(std::shared_ptr<BaseHandler> pCall);

  virtual ~Session();

  /*
  * @brief Init a session
  *   Init will set basic info for a session,
  *   including its process chain
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  void init();

  /*
  * @brief Session process it self
  *   Session has all the infomation that needed to process itself
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  void Process();

  /*
  * @brief Some new steps maybe added for a session
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  void AddChain(const ProcType type);

  /*
  * @brief If a session has next processer
  * If not, it should be finalize
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  bool HasNextProcessor() { return !proc_chain_.empty(); }

  /*
  * @brief Get next process step
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  ProcType NextProcessor();

  /*
  * @brief Get current process step
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  ProcType currentType() { return proc_chain_.front(); }

  /*
  * @brief End a session
  *  It is the only interface to end a session
  * @author mengmeng.zhi
  * @date 25/Dec/2018
  */
  void Final(int err);

 public:
  /*How many times will retry for a error http request*/
  ::size_t http_retry_time_;
  /*In which step a session is on*/
  ProcType current_type_;

  /*There may be more than one picuture to be downloaded, record the current url
   * index*/
  std::string current_download_url_;
  ::size_t current_download_url_index_;
  ::size_t downloadFailed_;

  /*There may be more than one picuture to be downloaded, record the current url
   * index*/
  // std::string current_download_url_;
  ::size_t url_count_;

 public:
  /*Unique ID for a session, it was assinged by msgid*/
  std::string uid_;

  std::string session_id_;

  /*Save downloaded imanges*/
  std::vector<ImageInfo> images_;

  std::string output_;

  TrackResult track_result_;

  PersonResult person_result_;

  std::string trace_space_result_;

  std::shared_ptr<BaseHandler> pHandler_;

  /*Store the process steps of a session*/
  std::list<ProcType> proc_chain_;

  SaveMongoData saveMongoData_;

 private:
  /*If a session is finished, its resouce will be released*/
  bool is_finished;
};
}

#endif  // FACEID_COMMON_SESSION_H
