//
// Created by jianbo on 3/24/18.
//

#ifndef PICSERVER_LOG_MODULE_H
#define PICSERVER_LOG_MODULE_H

#include "logmodule/hobotlog.h"
#include "chrono"

// for test multi-thread logging, need get kernel thread-id, not POSIX thread-id
extern pid_t gettid();

extern std::shared_ptr<hobotlog::Logger> g_logger;

#define LOG_TRACE(format, ...) g_logger->trace(format, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...) g_logger->debug(format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) g_logger->info(format, ##__VA_ARGS__)
#define LOG_WARN(format, ...) g_logger->warn(format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) g_logger->error(format, ##__VA_ARGS__)
#define LOG_CRITICAL(format, ...) g_logger->critical(format, ##__VA_ARGS__)

#define LOG_TRACE_FUNC(format, ...) \
  LOG_TRACE("{} " format, __FUNCTION__, ##__VA_ARGS__)

#define LOG_TRACE_EX(format, ...)                                       \
  LOG_TRACE("[{}:{}] " format, ConstExpr::filename(__FILE__), __LINE__, \
            ##__VA_ARGS__)
#define LOG_DEBUG_EX(format, ...)                                       \
  LOG_DEBUG("[{}:{}] " format, ConstExpr::filename(__FILE__), __LINE__, \
            ##__VA_ARGS__)
#define LOG_INFO_EX(format, ...)                                       \
  LOG_INFO("[{}:{}] " format, ConstExpr::filename(__FILE__), __LINE__, \
           ##__VA_ARGS__)
#define LOG_WARN_EX(format, ...)                                       \
  LOG_WARN("[{}:{}] " format, ConstExpr::filename(__FILE__), __LINE__, \
           ##__VA_ARGS__)
#define LOG_ERROR_EX(format, ...)                                       \
  LOG_ERROR("[{}:{}] " format, ConstExpr::filename(__FILE__), __LINE__, \
            ##__VA_ARGS__)
#define LOG_CRITICAL_EX(format, ...)                                       \
  LOG_CRITICAL("[{}:{}] " format, ConstExpr::filename(__FILE__), __LINE__, \
               ##__VA_ARGS__)

class PerfStat {
 public:
  PerfStat(std::string name, int id = 0) {
    func = name;
    msg_id = id;
    start = std::chrono::high_resolution_clock::now();
    // LOG_TRACE("[{}] enter", func);
  }

  ~PerfStat() {
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> cost = end - start;
    LOG_DEBUG("[{}] msg{} cost {}ms", func, msg_id, cost.count());
  }

  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  std::string func;
  int msg_id;
};

#define PerfDebugWithID(Name, id) PerfStat PerfStat##Name(Name, id)
#define PerfDebug(Name) PerfStat PerfStat##Name(Name, 0)
#endif  // PICSERVER_LOG_MODULE_H
