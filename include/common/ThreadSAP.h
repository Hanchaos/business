/***************************************************
*  描述:线程基类
*  日期:2018-11-16
*  作者:jun.chen@horizon.ai
*  说明:派生类继承实现Run即可创建线程对象
****************************************************/
#ifndef __THREAD_SAP_H__
#define __THREAD_SAP_H__

#include <thread>

class ThreadSAP {
 public:
  ThreadSAP();
  virtual ~ThreadSAP();

 public:
  static void* ThreadFunc(void* context);

 public:
  /** 创建一个 Worker 线程，并立即运行它 */
  bool CreateThread();
  /** 等待线程结束，如果线程未结束，将一直处于阻塞状态 */
  bool WaitForThreadExit();

 public:
  /** 线程实际运行的函数，继承类需重载此函数 */
  virtual void* Run();
  /** Sleep ms 毫秒 */
  void MySleep(unsigned int ms);

 protected:
  std::shared_ptr<std::thread> m_thread;
};

#endif
