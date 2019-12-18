#include "ThreadSAP.h"

ThreadSAP::ThreadSAP() {}

ThreadSAP::~ThreadSAP() {}

bool ThreadSAP::CreateThread() {
  m_thread.reset(new std::thread(std::bind(&ThreadSAP::ThreadFunc, this)));
  return true;
}

void* ThreadSAP::ThreadFunc(void* context) {
  ThreadSAP* pThis = (ThreadSAP*)context;
  return pThis->Run();
}

bool ThreadSAP::WaitForThreadExit() {
  if (m_thread) {
    if (m_thread->joinable()) {
      m_thread->join();
    }
  }
  return true;
}

void ThreadSAP::MySleep(unsigned int ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void* ThreadSAP::Run() { return 0; }
