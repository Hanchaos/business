#include "RedisClient.h"
#include "boost/algorithm/string.hpp"
#include "log_module.h"
#include <chrono>
#ifdef _WIN32
#include <Winsock2.h>

#endif /* _WIN32 */

CRedisClient::CRedisClient(QRedisParam param) {
  isOk_ = false;
  param_ = param;
}

CRedisClient::~CRedisClient() {}

bool CRedisClient::Init() { return InitClient(); }
bool CRedisClient::InitClient() {
  std::string addrs = param_.redis_addrs;
  if (addrs.empty()) {
    LOG_ERROR("[CRedisClient::InitClient] addrs is empty");
    return false;
  }

  try {
    //! Add your sentinels by IP/Host & Port
    std::vector<std::string> vecAddr;
    boost::split(vecAddr, addrs, boost::is_any_of(","));
    for (auto add : vecAddr) {
      std::vector<std::string> vIpPort;
      boost::split(vIpPort, add, boost::is_any_of(":"));
      if (vIpPort.size() != 2) {
        LOG_ERROR("[CRedisClient::InitClient] addrs is invalid {}", add);
        return false;
      }
      client_.add_sentinel(vIpPort[0], atoi(vIpPort[1].c_str()));
    }
    //! Call connect with optional timeout
    //! Can put a loop around this until is_connected() returns true.
    client_.connect(
        param_.sentinel_name,
        [](const std::string& host, std::size_t port,
           cpp_redis::client::connect_state status) {
          if (status == cpp_redis::client::connect_state::dropped) {
            LOG_ERROR(
                "[CRedisClient::InitClient] client disconnected from {}:{}",
                host, port);
          }
          LOG_WARN("[CRedisClient::InitClient] client status({}) from {}:{}",
                   (int)status, host, port);
        },
        0, -1, 5000);
    if (!client_.is_connected()) {
      LOG_ERROR("[CRedisClient::InitClient] client connect fail!");
      return false;
    }
    // set passwd
    bool bRet = true;
    if (param_.passwd != "") {
      client_.auth(param_.passwd, [&bRet](cpp_redis::reply& reply) {
        LOG_DEBUG("[CRedisClient::InitClient] redis auth : {}", reply);
        bRet = reply.ok();
      });
      client_.sync_commit();
    }
    if (!bRet) {
      LOG_ERROR("[CRedisClient::InitClient] redis auth fail");
      return false;
    }
    bRet = false;
    client_.select(param_.db_index, [&bRet](cpp_redis::reply& reply) {
      LOG_DEBUG("[CRedisClient::InitClient] select db {}", reply);
      bRet = reply.ok();
    });
    client_.sync_commit();
    if (!bRet) {
      // log here
      LOG_ERROR("[CRedisClient::InitClient] select db {} fail",
                param_.db_index);
      return false;
    }
  }
  catch (std::exception& ex) {
    LOG_ERROR("[CRedisClient::InitClient] exception fail!{}", ex.what());
    return false;
  }
  isOk_ = true;
  return true;
}

bool CRedisClient::OK() { return isOk_; }

cpp_redis::client& CRedisClient::get() { return client_; }

QRedisParam& CRedisClient::param() { return param_; }

bool CRedisClient::GetValueByKey(const std::string& key, std::string& data) {
  try {
    std::future<cpp_redis::reply> fut = client_.get(key);
    client_.commit();
    if (fut.valid()) {
      if (fut.wait_for(std::chrono::milliseconds(sync_wait_timeout_)) ==
          std::future_status::timeout) {
        LOG_ERROR("[ CRedisClient::GetValueByKey] get reply timeout");
        return false;
      } else {
        try {
          auto reply = fut.get();
          if (reply.is_null()) {
            LOG_ERROR("[ CRedisClient::GetValueByKey] reply object is null");
            data = "";
          } else {
            data = (reply.is_string() ? reply.as_string() : "");
            LOG_DEBUG(
                "[ CRedisClient::GetValueByKey] reply object is not null, "
                "success!");
          }
        }
        catch (std::exception& ex) {
          LOG_ERROR("[ CRedisClient::GetValueByKey] exception {}", ex.what());
          return false;
        }
      }
    } else {
      LOG_ERROR("[ CRedisClient::GetValueByKey] future is invalid");
      data = "";
    }
  }
  catch (std::exception& ex) {
    LOG_ERROR("[CRedisClient::GetValueByKey] exception {}", ex.what());
    return false;
  }
  if (data.length() == 0) return false;
  return true;
}

bool CRedisClient::SetAdvancedKeyValue(const std::string& key,
                                       const std::string& data, int timeout_ms,
                                       bool nx) {

  std::string set_rep = "";
  try {
    // EX=false; ex_sec=0; PS=true; ps_milli=EXPIRED_TIME; NX=true; XX=false
    // if not exist then create the key, otherwise return nil
    std::future<cpp_redis::reply> fut =
        client_.set_advanced(key, data, false, 0, true, timeout_ms, nx, false);
    client_.commit();
    if (fut.valid()) {
      if (fut.wait_for(std::chrono::milliseconds(sync_wait_timeout_)) !=
          std::future_status::timeout) {
        try {
          auto reply = fut.get();
          if (reply.is_null()) {
            LOG_ERROR(
                "[ CRedisClient::SetAdvancedKeyValue] reply object is null");
          } else {
            set_rep = (reply.is_string() ? reply.as_string() : "");
            LOG_DEBUG(
                "[ CRedisClient::SetAdvancedKeyValue] reply object is not "
                "null, "
                "success!");
          }
        }
        catch (std::exception& ex) {
          LOG_ERROR("[ CRedisClient::SetAdvancedKeyValue] exception {}",
                    ex.what());
        }
      } else {
        LOG_ERROR("[ CRedisClient::SetAdvancedKeyValue] get reply timeout");
      }
    } else {
      LOG_ERROR("[ CRedisClient::SetAdvancedKeyValue] future is invalid");
    }
  }
  catch (std::exception& ex) {
    LOG_ERROR("[CRedisClient::SetAdvancedKeyValue] exception {}", ex.what());
  }
  if (set_rep != "OK") {
    return false;
  } else {
    return true;
  }
}

bool CRedisClient::SetKeyValue(const std::string& key,
                               const std::string& data) {

  std::string set_rep = "";
  try {
    std::future<cpp_redis::reply> fut = client_.set(key, data);
    client_.commit();
    if (fut.valid()) {
      if (fut.wait_for(std::chrono::milliseconds(sync_wait_timeout_)) !=
          std::future_status::timeout) {
        try {
          auto reply = fut.get();
          if (reply.is_null()) {
            LOG_ERROR("[ CRedisClient::SetKeyValue] reply object is null");
          } else {
            set_rep = (reply.is_string() ? reply.as_string() : "");
            LOG_DEBUG(
                "[ CRedisClient::SetKeyValue] reply object is not null, "
                "success!");
          }
        }
        catch (std::exception& ex) {
          LOG_ERROR("[ CRedisClient::SetKeyValue] exception {}", ex.what());
        }
      } else {
        LOG_ERROR("[ CRedisClient::SetKeyValue] get reply timeout");
      }
    } else {
      LOG_ERROR("[ CRedisClient::SetKeyValue] future is invalid");
    }
  }
  catch (std::exception& ex) {
    LOG_ERROR("[CRedisClient::SetKeyValue] exception {}", ex.what());
  }

  if (set_rep != "OK") {
    return false;
  } else {
    return true;
  }
}

bool CRedisClient::ExpireKey(const std::string& key, int timeout) {
  // set key expire time
  int expire_rep = 0;

  try {
    std::future<cpp_redis::reply> fut = client_.expire(key, timeout);
    client_.commit();
    if (fut.valid()) {
      if (fut.wait_for(std::chrono::milliseconds(sync_wait_timeout_)) !=
          std::future_status::timeout) {
        try {
          auto reply = fut.get();
          if (reply.is_null()) {
            LOG_ERROR("[ CRedisClient::ExpireKey] reply object is null");
          } else {
            expire_rep = (reply.is_integer() ? reply.as_integer() : 0);
            LOG_DEBUG(
                "[ CRedisClient::ExpireKey] reply object is not null, "
                "success!");
          }
        }
        catch (std::exception& ex) {
          LOG_ERROR("[ CRedisClient::ExpireKey] exception {}", ex.what());
        }
      } else {
        LOG_ERROR("[ CRedisClient::ExpireKey] get reply timeout");
      }
    } else {
      LOG_ERROR("[ CRedisClient::ExpireKey] future is invalid");
    }
  }
  catch (std::exception& ex) {
    LOG_ERROR("[CRedisClient::ExpireKey] exception {}", ex.what());
  }

  // expire_rep = 1 ok
  if (expire_rep != 1) {
    return false;
  } else {
    return true;
  }
}

bool CRedisClient::RedLock(const std::string& key, const std::string& data,
                           int64_t acquiretimeout, long expiretimeout) {

  std::chrono::time_point<std::chrono::high_resolution_clock> m_begin =
      std::chrono::high_resolution_clock::now();

  int64_t elapsed = 0;
  while (elapsed < acquiretimeout) {
    // Note: expiretimeout's dimension is MilliSecond
    if (SetAdvancedKeyValue(key, data, expiretimeout, true)) {
      return true;
    }

    // wait lock_check_period_ (ms) for next checking loop
    std::this_thread::sleep_for(std::chrono::milliseconds(lock_check_period_));

    elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - m_begin).count();
  }
  return false;
}

bool CRedisClient::RedUnlock(const std::string& key, const std::string& data) {
  // default: 0 for unlock err
  int set_rep = 0;
  try {
    std::vector<std::string> vec_keys;
    std::vector<std::string> vec_args;
    vec_keys.push_back(key);
    vec_args.push_back(data);

    // if key exists then delete it, otherwise return false.
    std::future<cpp_redis::reply> fut = client_.eval(
        "if redis.call(\"get\",KEYS[1]) == ARGV[1] then return "
        "redis.call(\"del\", KEYS[1]) else return 0 end",
        vec_keys.size(), vec_keys, vec_args);
    client_.commit();
    if (fut.valid()) {
      if (fut.wait_for(std::chrono::milliseconds(sync_wait_timeout_)) ==
          std::future_status::timeout) {
        LOG_ERROR("[ CRedisClient::RedUnlock] get reply timeout");
        return false;
      } else {
        try {
          auto reply = fut.get();
          if (reply.is_null()) {
            LOG_ERROR("[ CRedisClient::RedUnlock] reply object is null");
          } else {
            set_rep = (reply.is_integer() ? reply.as_integer() : 0);
            LOG_DEBUG(
                "[ CRedisClient::RedUnlock] reply object is not null, "
                "success!");
          }
        }
        catch (std::exception& ex) {
          LOG_ERROR("[ CRedisClient::RedUnlock] exception {}", ex.what());
          return false;
        }
      }
    } else {
      LOG_ERROR("[ CRedisClient::RedUnlock] future is invalid");
    }
  }
  catch (std::exception& ex) {
    LOG_ERROR("[CRedisClient::RedUnlock] exception {}", ex.what());
    return false;
  }

  if (set_rep) {
    // unlock success
    return true;
  } else {
    return false;
  }
}
