/*
 * Copyright 2019 <Copyright hobot>
 * @brief Message receiver and dispatcher, singleton
 * @author xuehuan.hong
 * @date 25/Jan/2019
 */

#ifndef MSGCONSUMER_H
#define MSGCONSUMER_H

#include <mutex>
#include <memory>
#include "ConsumeProcess.h"
#include "log/log_module.h"

class MsgConsumer {
 public:
  ~MsgConsumer() {}

  static MsgConsumer* get_instance() {
    if (NULL == m_pInst) {
      std::lock_guard<std::mutex> lck(m_mutex);
      if (NULL == m_pInst) {
        m_pInst = new (std::nothrow) MsgConsumer();
      }
    }

    return m_pInst;
  }

  bool Initialize() {
    m_consumeProcess.reset(new (std::nothrow) CConsumeProcess());
    if (!m_consumeProcess) {
      LOG_ERROR("m_consumeProcess create fail");
      return false;
    }
    if (!m_consumeProcess->Init()) {
      LOG_ERROR("m_consumeProcess init fail");
      return false;
    }
    return true;
  }
  
  bool Restart(){
    if (!m_consumeProcess) {
      LOG_ERROR("m_consumeProcess create fail");
      return false;
    }
    m_consumeProcess->Release();
    if (!m_consumeProcess->Init()) {
      LOG_ERROR("m_consumeProcess init fail");
      return false;
    }
    return true;
  }

 private:
  static MsgConsumer* m_pInst;
  static std::mutex m_mutex;
  MsgConsumer() {}

 public:
  std::shared_ptr<CConsumeProcess> m_consumeProcess;
};


#endif
