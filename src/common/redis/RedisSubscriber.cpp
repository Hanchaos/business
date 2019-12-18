#include "RedisSubscriber.h"
#include "boost/algorithm/string.hpp"
#include "log_module.h"
#ifdef _WIN32
#include <Winsock2.h>
#endif /* _WIN32 */

CRedisSubscriber::CRedisSubscriber(QRedisParam param,
                                   const std::string& sub_key,
                                   sub_callback sub_func) {
  isOk_ = false;
  param_ = param;
  sub_key_ = sub_key;
  sub_func_ = sub_func;
}

CRedisSubscriber::~CRedisSubscriber() {}

bool CRedisSubscriber::config_set(const std::string& host, std::size_t port,
                                  const std::string& param,
                                  const std::string& val,
                                  const std::string& password) {
  if (param.length() == 0 || val.length() == 0) {
    return false;
  }
  try {
    cpp_redis::client client;
    client.connect(host, port, [](const std::string& host, std::size_t port,
                                  cpp_redis::client::connect_state status) {
      if (status == cpp_redis::client::connect_state::ok) {
        std::cout << "client connect from " << host << ":" << port << std::endl;
      }
    });
    if (!client.is_connected()) {
      return false;
    }
    bool bRet = true;
    if (password != "") {
      client.auth(password, [&bRet](cpp_redis::reply& reply) {
        LOG_DEBUG("[CRedisSubscriber::InitClient] redis auth : {}", reply);
        bRet = reply.ok();
      });
      client.sync_commit();
    }
    if (!bRet) return false;

    bRet = false;
    client.config_set(param, val, [&bRet](cpp_redis::reply& reply) {
      bRet = reply.ok();
      std::cout << "config set reply " << std::endl;
    });
    client.sync_commit();
    return bRet;
  }
  catch (std::exception& ex) {
    LOG_ERROR("[CRedisSubscriber::config_set] exception fail!{}", ex.what());
    return false;
  }
  return false;
}

bool CRedisSubscriber::Init() {
  conf_param_ = param_.config_key;
  conf_val_ = param_.config_value;
  if (!InitSubscriber()) return false;
  SubEvent(sub_key_, sub_func_);
  return true;
}
bool CRedisSubscriber::InitSubscriber() {
  std::string addrs = param_.redis_addrs;
  if (addrs.empty()) {
    LOG_ERROR("[CRedisSubscriber::InitSubscriber] addrs is empty");
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
        LOG_ERROR("[CRedisSubscriber::InitSubscriber] addrs is invalid {}",
                  add);
        return false;
      }
      sub_.add_sentinel(vIpPort[0], atoi(vIpPort[1].c_str()));
    }
    //! Call connect with optional timeout
    //! Can put a loop around this until is_connected() returns true.
    sub_.connect(
        param_.sentinel_name,
        [this](const std::string& host, std::size_t port,
               cpp_redis::subscriber::connect_state status) {
          if (status == cpp_redis::subscriber::connect_state::dropped) {
            LOG_ERROR(
                "[CRedisSubscriber::InitSubscriber] subscriber disconnected "
                "from "
                "{}:{}",
                host, port);
          } else if (status == cpp_redis::subscriber::connect_state::ok) {
            // psubscribe master节点配置
            CRedisSubscriber::config_set(host, port, conf_param(), conf_val(),
                                         param().passwd);
          }
          LOG_WARN(
              "[CRedisSubscriber::InitSubscriber] subscriber status({}) from "
              "{}:{}",
              (int)status, host, port);
        },
        0, -1, 5000);
    if (!sub_.is_connected()) {
      LOG_ERROR("[CRedisSubscriber::InitSubscriber] subscriber connect fail!");
      return false;
    }
    // set passwd
    if (param_.passwd != "") {
      std::shared_ptr<std::promise<bool>> promiseObj =
          std::make_shared<std::promise<bool>>();
      std::future<bool> futureObj = promiseObj->get_future();
      sub_.auth(param_.passwd, [this, promiseObj](cpp_redis::reply& reply) {
        LOG_DEBUG("[CRedisSubscriber::InitSubscriber] redis auth : {}", reply);
        promiseObj->set_value(reply.ok());
      });
      sub_.commit();
      std::chrono::seconds span(5000);
      if (futureObj.wait_for(span) == std::future_status::timeout) {
        LOG_ERROR("[CRedisSubscriber::InitSubscriber] redis auth timeout 5s");
        return false;
      }
      bool bRet = futureObj.get();
      if (!bRet) {
        LOG_ERROR("[CRedisSubscriber::InitSubscriber] redis auth fail");
        return false;
      }
    }
  }
  catch (std::exception& ex) {
    LOG_ERROR("[CRedisSubscriber::InitSubscriber] exception fail!{}",
              ex.what());
    return false;
  }
  isOk_ = true;
  return true;
}

void CRedisSubscriber::SubEvent(const std::string& key, sub_callback callback) {
  if (sub_.is_connected()) {
    sub_.psubscribe(
        key, [callback](const std::string& chan, const std::string& msg) {
          SubResult res;
          res.oper_ = chan;
          res.key_ = msg;
          LOG_DEBUG("[CRedisSubscriber::SubEvent]redis event: oper{} key{}",
                    chan, msg);
          callback(res);
        });
    sub_.commit();
  }
}

bool CRedisSubscriber::OK() { return isOk_; }

cpp_redis::subscriber& CRedisSubscriber::get() { return sub_; }

QRedisParam& CRedisSubscriber::param() { return param_; }

std::string CRedisSubscriber::conf_param() { return conf_param_; }

std::string CRedisSubscriber::conf_val() { return conf_val_; }