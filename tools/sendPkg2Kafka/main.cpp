#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <sstream>
#include <cstring>
#include <stdlib.h>
#include <fstream>
#include "kafkaProducer.h"

// #define DEBUG 1

//---------------------------------------
bool g_stop = false;
const char* kConfigPath = "";

void log(std::string s) { std::cout << s << std::endl; }

std::string lltoString(long long t) {
  std::string result;
  std::stringstream ss;
  ss << t;
  ss >> result;
  return result;
}

std::string packetAddTime(std::string msg_) {
  struct timeval now_time;
  gettimeofday(&now_time, NULL);

  int64_t start_time_ms =
      ((long)now_time.tv_sec) * 1000 + (long)now_time.tv_usec / 1000;
  auto st = lltoString(start_time_ms);
  std::string resMsg = msg_ + st;
  log(resMsg);
}

void Stop(int) { g_stop = true; }

void readFile(std::string file_name, std::string& content) {
  std::ifstream ifs(file_name.c_str());
  std::string temp((std::istreambuf_iterator<char>(ifs)),
                   (std::istreambuf_iterator<char>()));
  content.assign(temp);
}

int main(int argc, char** argv) {
  const char* topic_name = "test";
  int64_t loop = 100000;
  const char* broker_list = "10.10.108.122:9092";
  char* key = "";
  int oc;
  bool what_msg = false;
  std::string json_file = "";
  // c 发送次数  k key  t top b broker
  // f: json file
  while ((oc = getopt(argc, argv, "sc:k:b:t:f:")) != -1) {

    switch (oc) {

      case 's':
        what_msg = true;
        break;
      case 'c':
        loop = atoll(optarg);
        break;
      case 'k':
        key = optarg;
        break;
      case 't':
        topic_name = optarg;
        break;
      case 'b':
        broker_list = optarg;
        break;
      case 'f':
        json_file = optarg;
        break;
    }
  }

  printf("loop:%d key:%s topic:%s broker:%s json_file:%s\n", loop, key,
         topic_name, broker_list, json_file.c_str());
  if (json_file.empty()) {
    std::cout << "parameter json_file not gived" << std::endl;
    return -1;
  }

  signal(SIGINT, Stop);
  signal(SIGALRM, Stop);

  KafkaProducer producer;
  if (!producer.Init(broker_list, topic_name)) {
    log("Failed to init");
    return 0;
  }
  std::cout << "Producer init is ok!" << std::endl;

  std::string msg_whole;
  readFile(json_file, msg_whole);
  std::string msg_part = R"(
        {
            'id': '',
            'ver': 'SMART-1.4.1-02',
            'did': '02d0012328ef1bee48f6',
            'tmst': 1543484901,
            'src': '',
            'dest': '',
            'bid': '',
            'uid': 'city_test',
            'pformat': 0,
            'ptype': 8,
            'payload': '{'capture':{'pesn':{'tkid':112,'face':[{'tmst':677923995644,'systm':1543484898,'uri':'oss-cn-beijing.aliyuncs.com/zsy-feitian-test1/02d0012328ef1bee48f6_10.64.31.58_201811291748212169_1077929205.jpg','pibx':[528,78,624,174],'fcbx':[556,94,605,152],'age':31,'sex':1}]}}}'
        }
    )";

  struct timeval now_time;
  gettimeofday(&now_time, NULL);
  int64_t begin_time_ms =
      ((long)now_time.tv_sec) * 1000 + (long)now_time.tv_usec / 1000;
  while (!g_stop) {
    if (g_stop) {
      break;
    }
    if (-1 == loop) {
      while (!g_stop) {
        struct timeval now_time;
        gettimeofday(&now_time, NULL);

        int64_t tmp =
            ((long)now_time.tv_sec) * 1000 + (long)now_time.tv_usec / 1000;
        auto st = lltoString(tmp);
        printf("time:%d\n", tmp);
        std::string newMsg;
        if (what_msg) {
          newMsg = msg_part + lltoString(tmp);
        } else {
          //	newMsg = msg_whole + lltoString(tmp);
          newMsg = msg_whole;
        }

#if DEBUG
        std::cout << newMsg << std::endl;
#else
        std::cout << "produce 1" << std::endl;
        if (!producer.Produce(newMsg.c_str(), newMsg.size(), key)) {
          std::cout << "Failed to produce" << std::endl;
          // Retry to produce
        }
#endif
        usleep(5000);
      }
    } else {
      for (int i = 0; i < loop; ++i) {
        if (g_stop) {
          break;
        }
        struct timeval now_time;
        gettimeofday(&now_time, NULL);

        int64_t tmp =
            ((long)now_time.tv_sec) * 1000 + (long)now_time.tv_usec / 1000;
        auto st = lltoString(tmp);
        printf("num:%d\n", i);
        // printf("time:%d\n", tmp);
        std::string newMsg;
        if (what_msg) {
          newMsg = msg_part + lltoString(tmp);
        } else {
          // newMsg = msg_whole + lltoString(tmp);
          newMsg = msg_whole;
        }
#if DEBUG
        std::cout << newMsg << std::endl;
#else
        std::cout << "produce 2" << std::endl;
        if (!producer.Produce(newMsg.c_str(), newMsg.length(), key)) {
          std::cout << "Failed to produce" << std::endl;
          // Retry to produce
        }
#endif
        usleep(5000);
      }
      break;
    }
  }

  producer.Uninit();
  gettimeofday(&now_time, NULL);
  int64_t end_time_ms =
      ((long)now_time.tv_sec) * 1000 + (long)now_time.tv_usec / 1000;

  std::cout << "total time " << end_time_ms - begin_time_ms << std::endl;

  return 0;
}
