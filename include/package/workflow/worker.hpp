/*
* Copyright 2019 <Copyright hobot>
* @brief Worker to process request
* @author mengmeng.zhi
* @date 25/Dec/2018
*/

#ifndef WORKER_H
#define WORKER_H

#include <memory>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

const ::size_t MAX_TASK_NUMBER = 5000;

namespace XService {

/*
* @brief A FIFO queue
* @author mengmeng.zhi
* @date 25/Dec/2018
*/
template <typename T>
class FIFOBuffer {
 public:
  FIFOBuffer() : max_size_(MAX_TASK_NUMBER) {}

  void setMaxQueueSize(int size) { max_size_ = size; }

  void Put(std::shared_ptr<T>& new_value) {
    std::unique_lock<std::mutex> lck(mtx_);
    cond_put_.wait(lck, [this] { return max_size_ > data_.size(); });
    data_.push(new_value);
    cond_get_.notify_one();
  }

  std::shared_ptr<T> Get() {
    std::unique_lock<std::mutex> lck(mtx_);
    cond_get_.wait(lck, [this]() { return !this->data_.empty(); });
    std::shared_ptr<T> temp = data_.front();
    data_.pop();
    cond_put_.notify_one();

    return temp;
  }

 private:
  int max_size_;
  std::queue<std::shared_ptr<T>> data_;
  std::mutex mtx_;
  std::condition_variable cond_put_;
  std::condition_variable cond_get_;
};

/*
* @brief Threads with a FIFO queue
* @author mengmeng.zhi
* @date 25/Dec/2018
*/
template <typename T>
class Worker {
 private:
  explicit Worker(int thread_num = 1) {
    thread_num_ = thread_num;
    stop_ = false;
  }

  ~Worker() {}

 public:
  static Worker* getInstance() {
    if (NULL == pInstance_) {
      std::lock_guard<std::mutex> lck(singl_mutex_);
      if (NULL == pInstance_) {
        pInstance_ = new Worker();
      }
    }

    return pInstance_;
  }

  bool Initialize(int thread_numer, int maxTaskSize) {
    thread_num_ = thread_numer;
    queue_.setMaxQueueSize(maxTaskSize);

    return true;
  }

  void Start() {
    for (int i = 0; i < thread_num_; ++i) {
      threads_.emplace_back([this] {
        while (true) {
          if (this->stop_) {
            break;
          }

          std::shared_ptr<T> msg = queue_.Get();
          if (msg) {
            msg->Process();
          }
        }
      });
    }
  }

  virtual void Finalize() {
    stop_ = true;
    for (auto& thread : threads_) {
      thread.join();
    }
  }

  void PushMsg(std::shared_ptr<T>& msg) { queue_.Put(msg); }

 private:
  static std::mutex singl_mutex_;  // mutex for singleton
  static Worker* pInstance_;
  bool stop_;
  int thread_num_;
  std::vector<std::thread> threads_;
  FIFOBuffer<T> queue_;
};  // Worker

template <typename T>
Worker<T>* Worker<T>::pInstance_ = NULL;

template <typename T>
std::mutex Worker<T>::singl_mutex_;

}  // namespace XService

#define WORKER(T) Worker<T>::getInstance()

#endif  // WORKER_H
