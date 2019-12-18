#include <chrono>
#include <functional>
#include <sstream>
#include <string>
#include "kafka_consumer_imp.h"
#include "conf/conf_manager.h"
#include "com_kafka.h"

using namespace XService;

void log(std::string& msg) { std::cout << msg << std::endl; }

KafkaConsumerImp::KafkaConsumerImp(const std::string& brokers,
                                   FuncCallBack callback, int is_auto_commit)
    : m_broker_list(brokers),
      m_callback(callback),
      m_is_auto_commit(is_auto_commit),
      m_enable_conf_dump(false),
      m_run(false) {}

KafkaConsumerImp::~KafkaConsumerImp() { release(); }

bool KafkaConsumerImp::init() {
  std::string log_msg =
      RdKafka::version_str() + "" + std::to_string(RdKafka::version());
  log(log_msg);

  std::string errstr;
  m_conf.reset(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
  std::shared_ptr<RdKafka::Conf> topic_conf(
      RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC));
  m_conf->set("rebalance_cb", &m_rebalance_cb, errstr);
  m_conf->set("metadata.broker.list", m_broker_list, errstr);

  m_conf->set("event_cb", &m_event_cb, errstr);

  if (!m_is_auto_commit) {
    m_conf->set("enable.auto.commit", "false", errstr);
    // earliest: automatically reset the offset to the smallest offset
    topic_conf->set("auto.offset.reset", "earliest", errstr);
  }

  // print config if 'enable_conf_dump=true' in config file, default: false
  std::string s_conf_dump;
  int ret = CONF_PARSER()->getValue(kServerSection, kConfDump, s_conf_dump);
  if (0 == ret && "true" == s_conf_dump) {
    m_enable_conf_dump = true;
  }
  if (m_enable_conf_dump) {
    int pass;

    for (pass = 0; pass < 2; pass++) {
      std::list<std::string>* dump;
      if (pass == 0) {
        dump = m_conf->dump();
        std::cout << "# Global config" << std::endl;
      } else {
        dump = topic_conf->dump();
        std::cout << "# Topic config" << std::endl;
      }

      for (std::list<std::string>::iterator it = dump->begin();
           it != dump->end();) {
        std::cout << *it << " = ";
        it++;
        std::cout << *it << std::endl;
        it++;
      }
      std::cout << std::endl;
    }
  }

  m_conf->set("default_topic_conf", topic_conf.get(), errstr);

  return true;
}

void KafkaConsumerImp::release() {
  shutdown();
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  waitForThreadExit();
  if (m_consumer) m_consumer->close();
  RdKafka::wait_destroyed(3000);
}

bool KafkaConsumerImp::subscribe(const std::string& group,
                                 const std::vector<std::string>& topics) {
  std::string errstr;
  m_conf->set("group.id", group, errstr);

  m_consumer.reset(RdKafka::KafkaConsumer::create(m_conf.get(), errstr));

  if (!m_consumer) {
    std::string msg = std::string("Failed to create consumer: ") + errstr;
    log(msg);
    exit(1);
  }
  std::cout << " Created consumer " << m_consumer->name() << std::endl;

  RdKafka::ErrorCode err = m_consumer->subscribe(topics);
  if (err) {
    std::stringstream ss;
    ss << "Failed to subscribe to " << topics.size()
       << " topics: " << RdKafka::err2str(err) << std::endl;
    std::string msg;
    ss >> msg;
    log(msg);
    return false;
  }
  return true;
}

bool KafkaConsumerImp::resume() {
  if (m_cur_state == CONSUEMR_STATE_RUN) return false;
  std::vector<RdKafka::TopicPartition*> partitions;
  m_consumer->assignment(partitions);
  m_consumer->resume(partitions);
  m_cur_state = CONSUEMR_STATE_RUN;
  return true;
}

bool KafkaConsumerImp::pause() {
  if (m_cur_state == CONSUEMR_STATE_PAUSE) return false;
  std::vector<RdKafka::TopicPartition*> partitions;
  m_consumer->assignment(partitions);
  m_consumer->pause(partitions);
  m_cur_state = CONSUEMR_STATE_PAUSE;
  return true;
}

void KafkaConsumerImp::startup() {
  m_run = true;
  m_thread.reset(
      new std::thread(std::bind(&KafkaConsumerImp::threadFun, this)));
}

void KafkaConsumerImp::shutdown() { m_run = false; }

void KafkaConsumerImp::commit() {
  if (m_consumer) {
    m_consumer->commitSync();
  }
}

void KafkaConsumerImp::commit(RdKafka::Message* msg) {
  if (m_consumer && msg) {
    m_consumer->commitSync(msg);
  }
}

void KafkaConsumerImp::waitForThreadExit() {
  if (m_thread) {
    if (m_thread->joinable()) {
      m_thread->join();
    }
  }
}

void KafkaConsumerImp::threadFun() {
  /* TODO:
   * Accumulate a batch of batch_size messages, but wait no longer than
   * batch_tmout milliseconds.
   * Reference:
   * https://github.com/edenhill/librdkafka/blob/master/examples/rdkafka_consume_batch.cpp
  */
  while (m_run) {
    RdKafka::Message* message = m_consumer->consume(10);
    if (message && !message->err() && message->len() > 0) {
      //投递给工作线程
      std::shared_ptr<RdKafka::Message> msg(message);
      m_callback(msg);
    } else {
      if (message) delete message;
    }
  }
}
