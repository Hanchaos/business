//
// Created by jianbo on 4/17/18.
//

#ifndef PICSERVER_COMM_FUNC_H
#define PICSERVER_COMM_FUNC_H

#include "json/json.h"

namespace hobotcommon {
bool parse_json(const std::string &in_str, Json::Value &out_jv);

std::string generate_token(const std::string &info);

std::string calc_md5(const std::string &info1, const std::string &info2,
                     const std::string &info3);

time_t conver_time(const char *szTime);

std::string base64_encode(unsigned char const *bytes_to_encode,
                          unsigned int in_len);

std::string base64_decode(std::string const &encoded_string);

unsigned int BKDRHash(const char *str, unsigned int m = 1999);

unsigned int EasyHash(const char *str);

int64_t getMilliSecond();

void getmac(std::string *outmac);

std::string generate_process_name();

std::string generate_id(const std::string &prefix, long long time_stamp,
                        const std::string &device_id, int64_t track_id);

std::string getsplitstr(std::string str);

/*Get age index*/
int Age(int manual_age);
}
#endif  // PICSERVER_COMM_FUNC_H
