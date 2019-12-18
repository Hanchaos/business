#ifndef __XBUSINESS_HANDLER_H__
#define __XBUSINESS_HANDLER_H__
#include "DataDefine.h"

class CXBusinessHandler {
 public:
  CXBusinessHandler();
  ~CXBusinessHandler();
  static CXBusinessHandler* getInstance() {
    static CXBusinessHandler m_instance;
    return &m_instance;
  }

 public:
  virtual bool handle(std::shared_ptr<PM_DATA> data, std::string payload);
  bool handleFromRedis(std::string payload, std::string &pic_result);
};

#endif
