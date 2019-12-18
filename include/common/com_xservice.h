//
// Created by jianbo on 4/17/18.
//

#ifndef COMMON_XSERVICE_H
#define COMMON_XSERVICE_H

#include "json/json.h"
#include <string>

namespace XService {
struct RequestType {
  enum type {
    Surveillance = 1,
    Trace = 2,
    AutoRegister = 3,
    Screenshot = 6,  // business screenshot
    BusinessAnalyze = 7
  };
};

enum PayloadType {
  Screenshot = 1,  //商业背景截图
  xiashunBusiness = 128,  //夏舜进店客流
  xiashunCapture = 64,  //夏舜抓拍
  Trace = 8,       // trace使用
  Business = 32    // business
};

enum MsgType {
  PassengerFlow = 0,
  PersonAttribute = 1,
  BusinessCapture = 2
};

const int COMMON_CAPTURE = 8;
const int SMART_CAPTURE = 32;

const double MATCH_THRES_NOT_SET = -1;
const double REG_THRES_NOT_SET = -1;

const std::string kWarehouseSection = "warehouse";
const std::string kSetName = "set_name";
const std::string kVersion = "version";
const std::string kHousePath = "house_path";
const std::string kMongoSection = "mongo";
const std::string kMongoUrl = "mongo_url";
const std::string kMongoDbName = "mongo_dbname";
static const std::string kMongoBusinessSection = "mongo_business";

const char *const kEndPoint = "endpoint";
const char *const kBucket = "bucket";
const char *const kKey = "key";

const std::string kS3ProxySection = "s3proxy";
const std::string kPath = "path";
const std::string kGenUrlPath = "gen_url_path";
const std::string kIPPort = "ipport";

const char *const kImages = "images";
const char *const kImageBase64 = "image_base64";
const char *const kFaceAttr = "face_attr";
const char *const kFeature = "feature";
const char *const kAutoreg = "autoreg";
const char *const kActiveFlag = "active_flag";
const char *const kSets = "sets";
const char *const kFeatures = "features";

const char *const kResult = "result";
const char *const kErrorCode = "error_code";
const char *const kErrorMsg = "error_msg";
const char *const kIsMatched = "is_matched";
const char *const kAutoregisterFlag = "autoregister_flag";
const char *const kItems = "items";
const char *const kID = "id";
const char *const kDistance = "distance";
const char *const kSimilar = "similar";
const char *const kUrls = "urls";
const char *const kCTime = "ctime";
const char *const kATime = "atime";
const char *const kAttribute = "attribute";
const char *const kType = "type";
const char *const kValue = "value";
const char *const kFacesResult = "faces_result";
const char *const kFacesInfo = "faces_info";
const char *const kTraceResult = "track_result";
const char *const kPersonResult = "person_result";
const char *const kAutoRegResult = "autoreg_result";
const char *const kAutoRegScore = "autoreg_score";
const char *const kCanAutoReg = "can_autoreg";
const char *const kCanRegImg = "can_reg_img";
const char *const kImgScores = "img_scores";
const char *const kFaceRect = "face_rect";

const std::string kCommonSection = "common";
const std::string kTotalTimeout = "timeout";
const std::string kHttpRetryTime = "httpretrytime";
const std::string kTopic = "topic";
const std::string kDetectSection = "detect";
const std::string kThreadNum = "threadnum";
const std::string kSearchSection = "search";
const std::string kKafkaSection = "kafka";
const std::string kBrokers = "brokers";
const std::string kEnableFaceRect = "enable_ipc_facerect";

// const data for parsing
const char *const kuid = "uid";
const char *const kptype = "ptype";
const char *const kmsgid = "msg_id";
const char *const ktasks = "tasks";
const char *const kdata = "data";
const char *const kpayload = "payload";
const char *const kversion = "ver";
const char *const kdeviceid = "did";
const char *const ktimestamp = "tmst";
const char *const ktaskid = "task_id";
const char *const kbid = "bid";
const char *const kspaceid = "space_id";
const char *const kappid = "app_id";

const char *const klevelonename = "levelone_name";
const char *const kleveltwoname = "leveltwo_name";
const char *const kleveloneid = "levelone_id";
const char *const kleveltwoid = "leveltwo_id";


const char *const kcapture = "capture";
const char *const kpesn = "pesn";
const char *const ktkid = "tkid";
const char *const kface = "face";
const char *const ktmst = "tmst";
const char *const ksystm = "systm";
const char *const kuri = "uri";
const char *const kpibx = "pibx";
const char *const kfcbx = "fcbx";
const char *const kage = "age";
const char *const ksex = "sex";
const char *const kbkgd = "bkgd";

const char *const kresult = "result";
const char *const ktagt = "tagt";
const char *const ktype = "type";
const char *const kldmk = "ldmk";
const char *const ktkbx = "tkbx";
const char *const kpoint = "point";
const char *const kx = "x";
const char *const ky = "y";
const char *const kpose = "pose";

const char *const kevent = "event";
const char *const kgroupid = "group_id";
const char *const kextra = "extra";
const char *const keventtime = "event_time";
const char *const ktime = "timestamp";

const char *const ksendtime = "send_time";
const char *const klineid = "line_id";
const char *const keventtype = "event_type";
const char *const kflowtype = "flow_type";

const char *const kmsgtype = "msg_type";
const char *const ktrackid = "track_id";
const char *const kpersonid = "person_id";
const char *const kcaptureuris = "capture_uris";
const char *const kgender = "gender";

const char *const kThreshold = "threshold";
const char *const kDistanceThres = "distance_threshold";
const char *const kSimilarThres = "matched_similar";
const std::string kCityGroupID = "city";
const char *const kUrl = "url";
const char *const kIsManual = "is_manual";

const std::string kBusinessCaptureSection = "business_capture";
const std::string kCommonCaptureSection = "common_capture";
const std::string kPassengerFlowSection = "passenger_flow";
const std::string kScreenshotSection = "screenshot";

const std::string kWorkThreadNum = "worker_thread_num";
const std::string kMaxTaskNum = "max_task_num";

const int kCanRecognize = 0;
const int kCanRegist = 1;
const int kCanNotRecognize = 2;

const std::string kRedisSection = "redis";
const std::string kSentinel = "sentinel_name";
const std::string kAddrs = "addrs";
const std::string kPasswd = "passwd";
const std::string kIoNum = "io_num";
const std::string kSubKey = "sub_key";
const std::string kConfigKey = "config_key";
const std::string kConfigValue = "config_value";
const std::string kDbIndex = "db_index";
const std::string kPicIndex = "pic_index";
const std::string kRefreshInterval = "refresh_interval";
const char *const kreplay = "replay";

const std::string kSrcFromRedis = "redis";
const std::string kSrcFromKafka = "kafka";
}
#endif  // COMMON_XSERVICE_H
