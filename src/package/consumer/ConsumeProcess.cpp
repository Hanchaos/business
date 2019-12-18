#include <thread>
#include <boost/algorithm/string.hpp>
#include <future>
#include <vector>
#include "ConsumeProcess.h"
#include "log_module.h"
#include "json/json.h"
#include "conf/conf_manager.h"
#include "com_kafka.h"

using namespace XService;

FuncCallBack CConsumeProcess::m_process_func = NULL;

CConsumeProcess::CConsumeProcess() {}

CConsumeProcess::~CConsumeProcess() { Release(); }

bool CConsumeProcess::gracefully_exit() {
  if (nullptr != m_kafka_consumer) {
    m_kafka_consumer.reset();
  }
  return false;
}

bool CConsumeProcess::Init() {
  m_process_func =
      std::bind(&CConsumeProcess::process, this, std::placeholders::_1);
  // new kafka consumer
  std::string brokers;
  int ret = CONF_PARSER()->getValue(kServerSection, kRcvBrokers, brokers);
  if (0 != ret) {
    LOG_ERROR("conf file error: {} {}", kServerSection, kRcvBrokers);
    return false;
  }
  m_kafka_consumer.reset(new KafkaConsumerImp(brokers, m_process_func));
  if (!m_kafka_consumer) {
    LOG_ERROR("m_kafka_consumer create fail!");
    return false;
  }
  if (!m_kafka_consumer->init()) {
    LOG_ERROR("m_kafka_consumer init fail!");
    return gracefully_exit();
  }
  // get commit num and max_cache_size from config file
  auto nCommitNum = CONF_PARSER()->getIntValue(kServerSection, kCommitNum, ret);
  if (0 != ret) {
    nCommitNum = KAFKA_COMMIT_NUM;
  }
  m_commit_num = nCommitNum;
  auto nMaxCacheSize =
      CONF_PARSER()->getIntValue(kServerSection, kMaxCacheSize, ret);
  if (0 != ret) {
    nMaxCacheSize = MAX_CACHE_SIZE;
  }
  m_max_cache_size = nMaxCacheSize;
  // get group and topics from config file
  std::string group;
  ret = CONF_PARSER()->getValue(kServerSection, kGroup, group);
  if (0 != ret) {
    LOG_ERROR("conf file error: {} {}", kServerSection, kGroup);
    return gracefully_exit();
  }
  std::string strTopics;
  ret = CONF_PARSER()->getValue(kServerSection, kTopics, strTopics);
  if (0 != ret) {
    LOG_ERROR("conf file error: {} {}", kServerSection, kTopics);
    return gracefully_exit();
  }
  std::vector<std::string> vTopics;
  boost::split(vTopics, strTopics, boost::is_any_of(","));
  if (vTopics.empty()) {
    LOG_ERROR("conf file error: {} {}", kServerSection, kTopics);
    return gracefully_exit();
  }
  if (!m_kafka_consumer->subscribe(group, vTopics)) {
    LOG_ERROR("m_kafka_consumer subscribe fail!");
    return gracefully_exit();
  }
  // create a detach thread, infinite loop: receive msg ->
  // CConsumeProcess::process(msg),
  // which will push msg to "queue"
  m_kafka_consumer->startup();

  // create a detach thread, infinite loop: scan all partition manager (ie.
  // CConsumeProcess::Run()) -> handle msg
  m_is_run = true;
  try {
    // CreateThread only return true
    CreateThread();
  }
  catch (std::exception &e) {
    LOG_ERROR("CreateThread fail!");
    return gracefully_exit();
  }

  return true;
}

void CConsumeProcess::Release() {
  m_is_run = false;
  MySleep(300);
  WaitForThreadExit();
}

void CConsumeProcess::process(std::shared_ptr<void> msg) {
  std::shared_ptr<PM_DATA> objPtr(new PM_DATA);
  objPtr->msg = std::static_pointer_cast<RdKafka::Message>(msg);
  std::string strMsgId = "";
  Json::Value root;
  Json::Reader reader;
  try {
    if (!reader.parse((char *)objPtr->msg->payload(), root)) {
      LOG_ERROR("recv exception one:\r\n{}", (char *)objPtr->msg->payload());
      return;
    }
    strMsgId = root["msg_id"].asString();
  }
  catch (std::exception &e) {
    LOG_ERROR("recv exception one:\r\n{}", (char *)objPtr->msg->payload());
    return;
  }
  objPtr->msg_id = strMsgId;
  //添加到指定的PartitionManager
  int32_t nPartition = objPtr->msg->partition();
  LOG_DEBUG("uid[{}] partition[{}] recv one:\r\n{}", strMsgId, nPartition,
            (char *)objPtr->msg->payload());
  bool bNeedCreate = false;  //是否需要新创建分区管理
  {
    // change read lock to write lock
    boost::unique_lock<boost::shared_mutex> lock(m_partition_managers_mutex);
    auto it = m_partition_managers.find(nPartition);
    if (it != m_partition_managers.end()) {
      it->second->emplace_back(objPtr);
    } else {
      bNeedCreate = true;
    }
  }
  if (bNeedCreate) {
    std::shared_ptr<CPartitionManager> pmPtr(
        new CPartitionManager(m_kafka_consumer, m_commit_num));
    pmPtr->emplace_back(objPtr);
    boost::unique_lock<boost::shared_mutex> lock(
        m_partition_managers_mutex);  //写Lock
    m_partition_managers[nPartition] = pmPtr;
  }
}

void *CConsumeProcess::Run() {
  while (m_is_run) {
    std::mutex all_status_mutex;
    std::map<int32_t, PM_STATUS> all_status;
    {
      boost::shared_lock<boost::shared_mutex> lock(
          m_partition_managers_mutex);  //读Lock
      std::vector<std::future<int> > dofuts;
      for (auto it = m_partition_managers.begin();
           it != m_partition_managers.end(); it++) {
        dofuts.emplace_back(std::async(std::launch::async, [&, it] {
          PM_STATUS status;
          it->second->do_something(status);
          all_status_mutex.lock();
          all_status[it->first] = status;
          all_status_mutex.unlock();
          // LOG_DEBUG("partition manager({}) status: todo({}) doing({})
          // done({}) size({}) todo_to_doing({}) commit_num({})",
          //    it->first, status.todo_num, status.doing_num, status.done_num,
          // status.cache_size, status.process_num, status.commit_num);
          return it->first;
        }));
      }
      for (auto it = dofuts.begin(); it != dofuts.end(); it++) {
        if (it->wait_for(std::chrono::seconds(30)) ==
            std::future_status::timeout)
          LOG_ERROR("partition[{}] do_something 30s timeout ...", it->get());
      }
    }
    //信息日志输出
    int total_cache_size = 0;
    for (auto it = all_status.begin(); it != all_status.end(); it++) {
      total_cache_size += it->second.cache_size - it->second.done_num;
    }
    //根据缓存大小控制消费
    if (total_cache_size >= m_max_cache_size) {
      if (m_kafka_consumer->pause()) LOG_DEBUG("kafka consumer pause");
    } else {
      if (m_kafka_consumer->resume()) LOG_DEBUG("kafka consumer resume");
    }
    MySleep(50);
  }
  return NULL;
}
