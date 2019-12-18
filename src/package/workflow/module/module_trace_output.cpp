/*
* Copyright 2019 <Copyright hobot>
* @brief Operate the doc infomation of a person
* @author mengmeng.zhi
* @date 25/Dec/2018
*/
#include <mongocxx/cursor.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/client.hpp>
#include <boost/algorithm/string.hpp>
#include <time.h>
#include "tracktimer/tracktimer.h"

#include "log/log_module.h"
#include "common/com_func.h"
#include "workflow/module/module_trace_output.h"
#include "conf/conf_manager.h"
#include "json/json.h"
#include "xsearch/xsearch.h"
#include "common/com_func.h"
#include <xsearch/common/xsearch_data.h>
#include <logmodule/hobotlog.h>

using namespace XService;

static const std::string kWarehouseSection = "warehouse";
static const std::string kSetName = "set_name";
static const std::string kVersion = "version";
static const std::string kHousePath = "house_path";
static const std::string kMongoSection = "mongo";
static const std::string kMongoUrl = "mongo_url";
static const std::string kMongoDbName = "mongo_dbname";

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::array_context;
using bsoncxx::types::b_binary;
using bsoncxx::binary_sub_type;

static mongocxx::instance inst_{};

int GetDigit(std::string str);

int Age(int);

TraceSave::TraceSave() {
  module_name_ = "TraceSave";
  module_type_ = kTraceSave;
};

TraceSave::~TraceSave() {}

bool TraceSave::Initialize() {
  int ret =
      CONF_PARSER()->getValue(kWarehouseSection, kVersion, model_version_);
  if (0 != ret) {
    LOG_ERROR("[TraceSaveModule::Initialize] mongo version is null");
    return false;
  }
  ret = CONF_PARSER()->getValue(kWarehouseSection, kHousePath, house_name_);
  if (0 != ret) {
    LOG_ERROR("[TraceSaveModule::Initialize] mongo house_path is null");
    return false;
  }
  ret = CONF_PARSER()->getValue(kWarehouseSection, kSetName, set_name_);
  if (0 != ret) {
    LOG_ERROR("[TraceSaveModule::Initialize] mongo set_name is null");
    return false;
  }
  ret = CONF_PARSER()->getValue(kMongoSection, kMongoUrl, mongo_url_);
  if (0 != ret) {
    LOG_ERROR("[TraceSaveModule::Initialize] mongo url is null");
    return false;
  }

  mongo_dbname_ = "x-trace";
  ret = CONF_PARSER()->getValue(kMongoSection, kMongoDbName, mongo_dbname_);
  try {
    pool_ = std::move(std::unique_ptr<mongocxx::pool>(
        new mongocxx::pool(mongocxx::uri(mongo_url_))));
    if (!pool_) {
      LOG_INFO("[TraceSaveModule::Initialize] fail to get a mongo pool of {}",
               mongo_url_);
    }
  }
  catch (mongocxx::exception &e) {
    LOG_ERROR("[TraceSaveModule::Initialize] fail to get a mongo pool of {}",
              mongo_url_);
    return false;
  }
  ret = XSearchInit(set_name_, model_version_, house_name_);
  if (-1 == ret) {
    LOG_ERROR("[TraceSaveModule::Initialize] fail to init xsearch");
  } else {
    LOG_INFO("[TraceSaveModule::Initialize] init success");
  }
  return true;
}

int TraceSave::XSearchInit(std::string set_name, std::string model_version,
                           std::string house_name) {
  hobotlog::ResultCode errcode;
  xsearch::HobotInit(g_logger);
  // 构造底库数据源信息，从文件加载
  xsearch::HobotDataSourceInfo data_source_info;
  data_source_info.type_ = xsearch::HobotDataSourceType::FILE;
  data_source_info.BaseLibraryZipInfo.serialize_type_ =
      xsearch::HobotSerializeType::CVS;
  data_source_info.BaseLibraryZipInfo.file_name_ = house_name_;  // 底库文件
  //构造分片信息，
  xsearch::HobotSliceInfo slice_info;
  slice_info.type_ = xsearch::HobotSliceRuleType::LOGIC;
  xsearch::HobotResult set_result;
  // 新增一个名为test的FeatureSet到检索SDK中
  xsearch::HobotNewFeatureSet(set_name, data_source_info, slice_info,
                              &set_result, model_version_, 256);
  if (set_result.resp_type_ == xsearch::RespT::FAILED) {
    LOG_ERROR("[trace::XSearchInit]: error: {}", set_result.errmsg_);
    return -1;
  }

  //构造加载事件
  xsearch::HobotXSearchEvent xSearchEvent;
  xSearchEvent.name_ = set_name_;
  xSearchEvent.version_ = model_version_;
  xSearchEvent.event_type_ = xsearch::HobotEventType::LOAD;  // 加载底库事件
  xSearchEvent.block_index_ =
      0;  // 加载block 0,会去匹配底库文件中名为0.xxx的文件
  xsearch::HobotResult event_result;

  xsearch::HobotDoEvent(xSearchEvent, &event_result);  // 加载
  if (event_result.resp_type_ == xsearch::RespT::FAILED) {
    LOG_ERROR("[trace::XSearchInit]: error: {}", event_result.errmsg_);
    return -1;
  }
}

void TraceSave::Process(std::shared_ptr<Session> session) {
  session->DoRecord(module_name_ + "::Process");
  session->timer_->SetBigTimer(module_name_);
  session->timer_->SetSelfBegin(module_name_);
  int ret = 0;
  if (session->pHandler_->KafkaData_->capture_.replay_ == 1 &&
      CheckInMongo(session)) {
  } else {
    int centor_id = GetCenterID(session);

    ret = SaveTraceMongo(session, centor_id);
  }
  session->timer_->SetSelfEnd(this->module_name_);

  if (0 != ret) {
    session->Final(ErrCode::kMongoFailed);
    return;
  }
  if (!session->HasNextProcessor()) {
    session->Final(ErrCode::kSuccess);

    return;
  }
  WORKER(Session)->PushMsg(session);
  return;
}

bool TraceSave::CheckInMongo(std::shared_ptr<Session> session) {
  try {
    // mongo_collection = "xtrace_" + session->task_.trace_.name_ + "_" +
    // model_version_;
    std::string mongo_collection =
        "xtrace_" + session->pHandler_->KafkaData_->task_.trace_.name_ + "_";

    auto conn = pool_->try_acquire();
    if (!conn) {
      LOG_ERROR(
          "[TraceSave::CheckInMongo] uid: {}, session_id: {}, fail to connect "
          "mongo: {}",
          session->uid_, session->session_id_, mongo_url_);
      return false;
    }

    mongocxx::client &client = **conn;
    mongocxx::database db = client[mongo_dbname_];
    mongocxx::collection coll = db[mongo_collection];

    auto id_dev_track_ctime =
        session->pHandler_->KafkaData_->ipc_.device_id_ + "_" +
        std::to_string(
            session->pHandler_->KafkaData_->capture_.person_.track_id_) +
        "_" + std::to_string(session->pHandler_->KafkaData_->ipc_.timestamp_);
    document builder{};
    builder << "id" << id_dev_track_ctime << "task_id"
            << session->pHandler_->KafkaData_->task_.task_id_;
    bsoncxx::document::value filter_value = builder << finalize;

    mongocxx::options::find find_option;
    document sort_builder{};
    sort_builder << "ctime" << -1;
    bsoncxx::document::value sort_option = sort_builder << finalize;

    find_option.sort(sort_option.view());

    auto result = coll.find_one(filter_value.view(), find_option);
    if (!result) return false;
    LOG_DEBUG(
        "[TraceSave::CheckInMongo] uid: {}, session_id: {}, id: {} don't need "
        "save in",
        session->uid_, session->session_id_, id_dev_track_ctime);
    return true;
  }
  catch (mongocxx::exception &e) {
    LOG_ERROR(
        "[TraceSave::CheckInMongo] uid: {}, session_id: {}, mongo error {}",
        session->uid_, session->session_id_, e.what());
    return false;
  }
}

int TraceSave::GetCenterID(std::shared_ptr<Session> session) {
  if (session->features_.size() == 0) {
    return -1;
  }

  //构造检索请求
  xsearch::HobotSearchRequest searchRequest;
  searchRequest.blocks_.push_back(
      0);  // 指定查询的block，不指定查询featureSet中所有的block
  searchRequest.top_n_ = 1;  // 返回top 1 相似的结果
  searchRequest.set_name_ = set_name_;
  searchRequest.model_version_ = model_version_;

  auto fun = [](std::string feature_str)->std::string {
    std::vector<std::string> temp;
    std::vector<float> feats;
    size_t last = 0;
    size_t index = feature_str.find_first_of(",", last);
    while (index != std::string::npos) {
      temp.push_back(feature_str.substr(last, index - last));
      last = index + 1;
      index = feature_str.find_first_of(",", last);
    }
    if (index - last > 0) {
      temp.push_back(feature_str.substr(last, index - last));
    }

    for (auto &item : temp) {
      std::stringstream ss(item);
      float feat;
      ss >> feat;
      feats.push_back(feat);
    }

    std::stringstream floatstream;
    for (const auto &each_f : feats) {
      floatstream.write((char *)(&each_f), sizeof(float));
    }
    std::string feat_str = floatstream.str();
    return feat_str;
  };

  for (auto &feat_str : session->features_) {
    std::string feat_bin_string = fun(feat_str.second);
    searchRequest.encrypt_feat_.push_back(feat_bin_string);
  }

  // 检索
  xsearch::HobotSearchResult searchResult;
  if (xsearch::HobotSearch(searchRequest, &searchResult) < 0) {
    LOG_ERROR("[TraceSave:GetCentID]uid:{} {} errmsg: {}",
              session->pHandler_->KafkaData_->msg_id_, session->session_id_,
              searchResult.errmsg_);
    return -1;
  }

  if (searchResult.id_score_.size() < 1) {
    LOG_ERROR("cluster searchResult is null");
    return -1;
  }
  std::string s_cluster_id = searchResult.id_score_[0].first;
  int32_t d_cluster_id = GetDigit(s_cluster_id);
  if (d_cluster_id < 0) {
    LOG_ERROR("[TraceSave::SaveTraceMongo] invalid cluster id {}",
              s_cluster_id);
    return -1;
  }
  return d_cluster_id;
}

bool TraceSave::IsRealMatch(std::shared_ptr<Session> session) {
  /*for (auto &ren : session->matched_infos_) {
    if (ren.similar >= session->task_.trace_.match_threshold_) {
      return true;
    }
  }
  return false;*/

  return session->is_xid_matched_;
}

int TraceSave::FillMatchInfo(std::shared_ptr<Session> session,
                             MatchPersonInfo &matched_person) {
  bool is_new_customer = false;
  if (session->is_auto_reg_success_) {
    matched_person.id = session->auto_reg_person_id_;
    matched_person.distance = 0;
    matched_person.similar = 1;
    matched_person.set_name =
        session->pHandler_->KafkaData_->task_.trace_.name_;
    matched_person.urls.push_back(
        session->pHandler_->KafkaData_->capture_.person_.faces_[0].url_);
    matched_person.ctime = std::to_string(time(NULL));
    return 2;
  }

  if (IsRealMatch(session)) {
    double max_similarity = 0;
    for (auto &person : session->matched_infos_) {
      if (person.similar > max_similarity) {
        max_similarity = person.similar;
        matched_person = person;
      }
    }
    //由于商业添加自注册，预防新客进来后商业先进行自注册导致建档误识别为老客
    auto match_urls = matched_person.urls;
    auto capture_faces =
        session->pHandler_->KafkaData_->capture_.person_.faces_;
    for (auto &person_face :
         capture_faces)  //判断matched_person
                         //url是否为抓拍url，若相等，该客户为新客。
    {
      auto capture_url = person_face.url_;
      for (auto &temp : match_urls) {
        if (temp == capture_url) {
          is_new_customer = true;
          break;
        }
      }
    }

    if (!is_new_customer) {
      if (matched_person.set_name !=
          session->pHandler_->KafkaData_->task_.trace_.name_) {
        return 1;
      } else {
        return 3;
      }
    } else {
      return 2;
    }
  }

  matched_person.id = hobotcommon::generate_id(
      "noreg_", session->pHandler_->KafkaData_->ipc_.timestamp_,
      session->pHandler_->KafkaData_->ipc_.device_id_,
      session->pHandler_->KafkaData_->capture_.person_.track_id_);
  matched_person.distance = 0;
  matched_person.similar = 1;
  return 0;
}

/*void split_to_float(const std::string &str,std::vector<float> &f_feature)
{
  size_t pos1 = 0;
  size_t len = str.length();
  if ("" == str) return;
  std::size_t pos2 = str.find(',');
  while (pos2 != std::string::npos)
  {
      f_feature.push_back(atof(str.substr(pos1, pos2 - pos1).c_str()));
      pos1 = pos2 + 1;
      pos2 = str.find(',',pos1);

  }
  if (pos1 != len)
  {
      f_feature.push_back(atof(str.substr(pos1).c_str()));
  }
}*/

int TraceSave::SaveTraceMongo(std::shared_ptr<Session> session,
                              int d_cluster_id) {
  try {
    //    mongo_collection = "xtrace_" + session->task_.trace_.name_ + "_" +
    // model_version_;
    std::string mongo_collection =
        "xtrace_" + session->pHandler_->KafkaData_->task_.trace_.name_ + "_";

    auto conn = pool_->try_acquire();
    if (!conn) {
      LOG_INFO("[TraceSave::SaveTraceMongo] {} fail to connect mongo {}",
               session->session_id_, mongo_url_);
      return -1;
    }

    mongocxx::client &client = **conn;
    mongocxx::database db = client[mongo_dbname_];
    mongocxx::collection coll = db[mongo_collection];
    if (!index_add_flag_[mongo_collection] &&
        !session->pHandler_->KafkaData_->task_.trace_
             .auto_reg) {  // 仅路人库在此增加索引，一人一档抓拍库索引在xtrace-api创建
      index_add_flag_[mongo_collection] = true;
      try {
        mongocxx::options::index index_options{};
        //          index_options.unique(true);
        //        db[_table_name].create_index(document{}<< "id" << 1
        // <<finalize,index_options);
        db[mongo_collection].create_index(
            document{} << "cluster_id" << 1 << "ctime" << -1 << finalize,
            index_options);
        db[mongo_collection].create_index(
            document{} << "device_id" << 1 << "task_id" << 1 << finalize,
            index_options);
      }
      catch (mongocxx::exception &e) {
        LOG_ERROR("[TraceSave::SaveTraceMongo] mongo error {}", e.what());
        return -1;
      }
    }

    auto id_dev_track_ctime =
        session->pHandler_->KafkaData_->ipc_.device_id_ + "_" +
        std::to_string(
            session->pHandler_->KafkaData_->capture_.person_.track_id_) +
        "_" + std::to_string(session->pHandler_->KafkaData_->ipc_.timestamp_);

    std::string s_score_img = session->xfr_result.img_scores;
    std::vector<std::string> v_score_img;
    boost::split(v_score_img, s_score_img, boost::is_any_of(","));

    auto builder = bsoncxx::builder::stream::document{};
    builder
        << "uid" << session->pHandler_->KafkaData_->ipc_.uid_ << "device_id"
        << session->pHandler_->KafkaData_->ipc_.device_id_ << "task_id"
        << session->pHandler_->KafkaData_->task_.task_id_
           //<< "set_name" << session->task_.trace_.category_sets_.front()
           ////modify by chenjun 2018-9-27
        << "set_name" << session->pHandler_->KafkaData_->task_.trace_.name_
        << "model_version" << model_version_ << "id" << id_dev_track_ctime
        << "manu_attribute" << open_array << close_array << "auto_attribute"
        << open_array << close_array << "track_id"
        << session->pHandler_->KafkaData_->capture_.person_.track_id_ << "ctime"
        << (int64_t)(session->pHandler_->KafkaData_->ipc_.timestamp_)
        << "cluster_id" << d_cluster_id << "logic_id" << (rand() % 10000)
        << "features" << open_array
        << [&](array_context<> arr) {
             // Attention: Follow in the right order:
             // firstly, record face with features; then, record face without
             // feature.
             for (const auto &couple : session->features_) {
               if (couple.first < session->images_.size()) {
                 /*std::vector<float> f_feature;
                 split_to_float(couple.second,f_feature);
                 b_binary feature_binary{binary_sub_type::k_binary,
                                         static_cast<uint32_t>(f_feature.size()*sizeof(float)),
                                         reinterpret_cast<const uint8_t
                 *>(f_feature.data())};*/
                 auto img_url = session->images_[couple.first].url_;
                 for (auto &face :
                      session->pHandler_->KafkaData_->capture_.person_.faces_) {
                   if (face.url_ == img_url) {
                     int age = Age(face.age_);
                     arr << open_document << "timestamp"
                         << (int64_t)(face.tmst_) << "capture_time"
                         << (int64_t)(face.systm_) << "url" << face.url_
                         << "feature" << couple.second << "feature_size" << 256
                         << "score" << atof(v_score_img[couple.first].c_str())
                         << "age" << session->xfr_result.age << "gender"
                         << session->xfr_result.gender << close_document;
                     face.extra_ = 1;
                     break;
                   }
                 }
               }
             }

             for (auto &face :
                  session->pHandler_->KafkaData_->capture_.person_.faces_) {
               if (face.extra_ == 0) {
                 int age = Age(face.age_);
                 arr << open_document << "timestamp" << (int64_t)(face.tmst_)
                     << "capture_time" << (int64_t)(face.systm_) << "url"
                     << face.url_ << "feature"
                     << ""
                     << "feature_size" << 0 << "score" << double(0) << "age"
                     << session->xfr_result.age << "gender"
                     << session->xfr_result.gender << close_document;
               }
             }
           } << close_array << "track" << open_document << "age"
        << session->xfr_result.age << "gender" << session->xfr_result.gender
        << "score" << session->xfr_result.autoreg_score << close_document;

    MatchPersonInfo matched_person;
    int person_type = FillMatchInfo(session, matched_person);
    builder << "person" << open_document << "id" << matched_person.id << "type"
            << person_type << "feature_set" << matched_person.set_name << "url"
            << (matched_person.urls.empty() ? "" : matched_person.urls.front())
            << "create_time" << atol(matched_person.ctime.c_str())
            << "last_cluster_time" << (int64_t)(-1) << close_document;

    builder << "last_mtime" << (int64_t)(-1) << "delete_flag" << 0;

    auto doc_val = builder << finalize;
    session->output_ = bsoncxx::to_json(doc_val.view());
    LOG_INFO("[TraceSave::SaveTraceMongo]uid:{} {} view {}",
             session->pHandler_->KafkaData_->msg_id_, session->session_id_,
             bsoncxx::to_json(doc_val.view()));
    coll.insert_one(doc_val.view());
    LOG_INFO("[TraceSave::SaveTraceMongo]uid:{} {} insert success",
             session->pHandler_->KafkaData_->msg_id_, session->session_id_);
  }
  catch (mongocxx::exception &e) {
    LOG_ERROR("[TraceSave::SaveTraceMongo] mongo error {}", e.what());
    return -1;
  }
  return 0;
}

int GetDigit(std::string str) {
  int sPos = -1;
  int ePos = -1;
  for (int i = 0; i < str.length(); i++) {
    if (str.at(i) >= '0' && str.at(i) <= '9') {
      sPos = i;
      break;
    }
  }
  if (sPos == -1) return -1;

  ePos = str.length();
  for (int i = sPos + 1; i < str.length(); i++) {
    if (str.at(i) < '0' || str.at(i) > '9') {
      ePos = i;
      break;
    }
  }
  std::string strDigit = str.substr(sPos, ePos - sPos);
  int32_t d_cluster_id = -1;
  std::stringstream ss_cluster_id(strDigit);
  ss_cluster_id >> d_cluster_id;
  return d_cluster_id;
}

int Age(int age) {
  int age_index = -1;
  if (age >= 0 && age < 6) {
    age_index = 0;
  } else if (age >= 6 && age < 12) {
    age_index = 1;
  } else if (age >= 12 && age < 18) {
    age_index = 2;
  } else if (age >= 18 && age < 28) {
    age_index = 3;
  } else if (age >= 28 && age < 35) {
    age_index = 4;
  } else if (age >= 35 && age < 45) {
    age_index = 5;
  } else if (age >= 45 && age < 55) {
    age_index = 6;
  } else if (age >= 55 && age < 100) {
    age_index = 7;
  }
  return age_index;
}
