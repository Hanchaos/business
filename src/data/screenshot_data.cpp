#include "screenshot_data.h"
#include "com_xservice.h"
#include <list>
#include "DataDefine.h"
#include <string>
#include "workflow/module_define.h"
#include "json/json.h"
#include "common/com_func.h"
#include "conf/conf_manager.h"
#include "workflow/module/module_kafka_output.h"

using namespace XService;

bool ScreenshootData::parse(void) {

  Json::Value data_jv;
  Json::Value tasks_jv;

  if (total_jv_.isMember(kdata)) {
    if (total_jv_[kdata].isObject()) {
      data_jv = total_jv_[kdata];
    } else if (total_jv_[kdata].isString()) {
      if (!hobotcommon::parse_json(total_jv_[kdata].asString(), data_jv))
        return false;
    } else {
      return false;
    }
  } else {
    return false;
  }

  if (total_jv_.isMember(kmsgid) && total_jv_[kmsgid].isString()) {
    msg_id_ = total_jv_[kmsgid].asString();
  }

  if (data_jv.isMember(kbid) && data_jv[kbid].isString()) {
    if (!hobotcommon::parse_json(data_jv[kbid].asString(), tasks_jv))
      return false;
  } else {
    return false;
  }

  Json::Value payload_jv;
  if (data_jv.isMember("payload") && data_jv["payload"].isString()) {
    payload_ = data_jv["payload"].asString();
    if (!hobotcommon::parse_json(payload_, payload_jv)) {
      LOG_ERROR("parse payload fail");
      return false;
    }
  } else {
    return false;
  }

  //解析ipc信息
  if (!parseIpcInfo(data_jv)) {
    return false;
  }

  //解析抓拍信息
  if (!ParseScrennshot(payload_jv)) {
    return false;
  }

  //解析任务
  if (!parseTasks(tasks_jv)) {
    return false;
  }

  return true;
}

bool ScreenshootData::ParseScrennshot(Json::Value &payload_jv) {

  Json::Value capture_jv;
  if (payload_jv.isMember(kcapture) && payload_jv[kcapture].isObject()) {
    capture_jv = payload_jv[kcapture];
  } else {
    return false;
  }

  CaptureInfo &capture = capture_;

  if (capture_jv.isMember(kreplay) && capture_jv[kreplay].isInt()) {
    capture.replay_ = capture_jv[kreplay].asInt();
  }

  if (capture_jv.isMember(kbkgd) && capture_jv[kbkgd].isObject()) {
    capture.type_ += 2;
    auto background = capture_jv[kbkgd];
    if (background.isMember(ktkid) && background[ktkid].isArray()) {
      for (auto j = 0; j < background[ktkid].size(); ++j) {
        capture.background_.track_ids_.push_back(background[ktkid][j].asUInt());
      }
    }
    if (background.isMember(ktmst) && background[ktmst].isInt64()) {
      capture.background_.tmst_ = background[ktmst].asInt64();
    }
    if (background.isMember(ksystm) && background[ksystm].isInt64()) {
      capture.background_.systm_ = background[ksystm].asInt64();
    }
    if (background.isMember(kuri) && background[kuri].isString()) {
      capture.background_.url_ = background[kuri].asString();
    }
  }

  return true;
}

bool ScreenshootData::makeProcessChain(std::list<ProcType> &procList) {
  procList.push_back(kOutputKafka);
  return true;
}

bool ScreenshootData::makeKafkaRequest(
    std::shared_ptr<Session> &session,
    std::list<KafkaOutputInfo> &ouput_info_list) {
  Json::Value out_jv;

  out_jv["device_id"] = ipc_.device_id_;
  out_jv["time"] = (int)ipc_.timestamp_;
  out_jv["ptype"] = payload_type_;
  out_jv["payload"] = payload_;

  for (auto &taskinfo : taskArry_) {
    if (taskinfo.type_ == RequestType::Screenshot &&
        taskinfo.extra_ != "city") {
      out_jv["app_id"] = taskinfo.uid_;
      KafkaOutputInfo tmp;
      tmp.data = out_jv.toStyledString();
      CONF_PARSER()->getValue(XService::kScreenshotSection, XService::kTopic,
                              tmp.topic);
      ouput_info_list.push_back(tmp);
      LOG_INFO(
          "[ScreenshootData::makeKafkaRequest]uid:{} session_id:{} kafka "
          "topic:{} output:{}",
          session->uid_, session->session_id_, tmp.topic, tmp.data);
    }
  }
  return true;
}

bool ScreenshootData::parseTasks(Json::Value &tasks_jv) {
  Json::Value task_jv;
  TaskInfo taskinfo;

  if (tasks_jv.isMember(ktasks) && tasks_jv[ktasks].isArray()) {
    auto tasks = tasks_jv[ktasks];
    for (int i = 0; i < tasks.size(); ++i) {
      auto task = tasks[i];
      if (!task.isObject()) {
        continue;
      }
      int request_type =
          task["busi_type"].isInt() ? task["busi_type"].asInt() : 0;
      std::string taskid =
          task["task_id"].isString() ? task["task_id"].asString() : "";
      std::string uid = task["uid"].isString() ? task["uid"].asString() : "";
      taskinfo.uid_ = uid;
      taskinfo.task_id_ = taskid;
      taskinfo.device_id_ =
          task["device_id"].isString() ? task["device_id"].asString() : "";
      taskinfo.extra_ = task["extra"].isString() ? task["extra"].asString() : "";
      taskinfo.type_ = request_type;
      taskArry_.push_back(taskinfo);
    }
    return true;
  } else {
    LOG_INFO("there exists no \"tasks\" or the type isn't array in kafka data");
    return true;
  }
}

