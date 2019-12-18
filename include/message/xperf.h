//
// Created by jianbo on 8/6/18.
//

#ifndef XBUSINESS_PERF_H
#define XBUSINESS_PERF_H

#include <list>
#include <string>
#include <utility>
#include <memory>
#include "log/log_module.h"
#include "tracktimer/tracktimer.h"

namespace XService {
class PerfTPInfo {
 public:
  explicit PerfTPInfo(std::string task_type_) {
    time_track_ = 0;
    timer_ = std::make_shared<tracktimer::TrackTimer>(task_type_);
  }

  void DoRecord(std::string key) {
#ifdef XPERF
    std::chrono::time_point<std::chrono::high_resolution_clock> tp =
        std::chrono::high_resolution_clock::now();
    records_.push_back(std::make_pair(key, tp));
#endif
  }

  void Print(std::string id, std::string uid) {
#ifdef XPERF
    std::string summery(id + " stream: ");
    std::chrono::time_point<std::chrono::high_resolution_clock> pre;
    auto iter = records_.begin();
    if (iter != records_.end()) {
      summery += iter->first;
      pre = iter->second;
      iter++;
      while (iter != records_.end()) {
        std::chrono::duration<double, std::milli> dur = iter->second - pre;
        summery += "--" + std::to_string(dur.count()) + "--";
        summery += iter->first;
        pre = iter->second;
        iter++;
      }
    }
    if (!summery.empty()) {
      LOG_DEBUG("{}", summery);
    }
#endif
    LOG_INFO("uid[{}] id[{}] summary: {}", uid, id, timer_->contenttojson());
  }

 public:
  int time_track_;
  std::list<std::pair<
      std::string, std::chrono::time_point<std::chrono::high_resolution_clock>>>
      records_;
  std::shared_ptr<tracktimer::TrackTimer> timer_;
};
}
#endif  // XBUSINESS_PERF_H
