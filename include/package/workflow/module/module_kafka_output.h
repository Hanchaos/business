//
// Created by jianbo on 7/28/18.
//

#ifndef XSERVICE_KAFKA_MODULE_H
#define XSERVICE_KAFKA_MODULE_H

#include <map>
#include <string>
#include <mutex>
#include "librdkafka/rdkafka.h"
#include "workflow/module_wrapper.h"

namespace XService {
class OutPutKafkaModule : public ModuleWrapper {
 public:
  OutPutKafkaModule();

  virtual ~OutPutKafkaModule();

  virtual bool Initialize();

  virtual void Process(std::shared_ptr<Session>);

  virtual void Finalize();

  bool push_data_to_kafka(std::string &, std::string &,
                          std::shared_ptr<Session>);

 private:
  void ProcError(std::shared_ptr<Session> session);

 public:
  std::string brokers_;
  rd_kafka_t *ptr_rk_;                     /* Producer instance handle */
  std::map<std::string, void *> rkt_dict_; /* Map task to topic */
  std::mutex mtx_;
};

class KafkaOutputInfo {
 public:
  std::string topic;
  std::string data;
};
}
#endif  // XSERVICE_KAFKA_MODULE_H
