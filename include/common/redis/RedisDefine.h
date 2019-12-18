/***************************************************
 *  描述:redis数据结构定义类
 *  日期:2019-01-26
 *  作者:jun.chen@horizon.ai
 *  说明:
 ****************************************************/
#ifndef __REDIS_DEFINE_H__
#define __REDIS_DEFINE_H__
#include <string>

typedef struct {
  std::string sentinel_name;  // 哨兵名
  std::string redis_addrs;    // redis地址集合
  int io_num;                 // io server个数，大于clinet个数
  int db_index;               // db index
  std::string passwd;         // redis密码
  std::string sub_key;        // 订阅key
  std::string config_key;
  std::string config_value;
} QRedisParam;

#endif