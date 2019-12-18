//
// Created by jianbo on 4/17/18.
//

#ifndef COMMON_COM_KAFKA_H
#define COMMON_COM_KAFKA_H

#include <string>

namespace XService {

// const variables for kafka, with which this server pulling data
const std::string kServerSection = "server";
const std::string kRcvBrokers = "brokers";
const std::string kGroup = "group";
const std::string kTopics = "topics";
const std::string kCommitNum = "commit_num";
const std::string kMaxCacheSize = "max_cache_size";
const std::string kConfDump = "enable_conf_dump";
const int KAFKA_COMMIT_NUM = 10;
const int MAX_CACHE_SIZE = 10000;
}

#endif  // COMMON_COM_KAFKA_H
