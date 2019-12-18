#include "base_data.h"
#include "com_xservice.h"
#include <list>

using namespace XService;

bool BaseData::parseIpcInfo(Json::Value &data_jv) {
  if (data_jv.isMember(kuid) && data_jv[kuid].isString()) {
    ipc_.uid_ = data_jv[kuid].asString();
  }
  if (data_jv.isMember(kversion) && data_jv[kversion].isString()) {
    ipc_.version_ = data_jv[kversion].asString();
  }
  if (data_jv.isMember(kdeviceid) && data_jv[kdeviceid].isString()) {
    ipc_.device_id_ = data_jv[kdeviceid].asString();
  }
  else {
    return false;
  }
  if (data_jv.isMember(ktimestamp) && data_jv[ktimestamp].isInt64()) {
    ipc_.timestamp_ = data_jv[ktimestamp].asInt64();
  }
  if (data_jv.isMember(kappid) && data_jv[kappid].isString()) {
    ipc_.app_id_ = data_jv[kappid].asString();
  }

  if (data_jv.isMember(kspaceid) && data_jv[kspaceid].isString()) {
    ipc_.space_id_ = data_jv[kspaceid].asString();
  }

  std::string levelone_name;
  std::string leveltwo_name;
  std::string levelone_id;
  std::string leveltwo_id;

  if (data_jv.isMember(klevelonename) && data_jv[klevelonename].isString()) {
    levelone_name = data_jv[klevelonename].asString();
  }

  if (data_jv.isMember(kleveltwoname) && data_jv[kleveltwoname].isString()) {
    leveltwo_name = data_jv[kleveltwoname].asString();
  }

  if (data_jv.isMember(kleveloneid) && data_jv[kleveloneid].isString()) {
    levelone_id = data_jv[kleveloneid].asString();
  }

  if (data_jv.isMember(kleveltwoid) && data_jv[kleveltwoid].isString()) {
    leveltwo_id = data_jv[kleveltwoid].asString();
  }

  if (levelone_name == "app_id" && levelone_id != "") {
    ipc_.app_id_ = levelone_id;
  }

  if (leveltwo_name == "space_id" && leveltwo_id != "") {
    ipc_.space_id_ = leveltwo_id;
  }

  if (ipc_.app_id_ == "") {
    return false;
  }
  
  if (ipc_.space_id_ == "") {
    ipc_.space_id_ = ipc_.app_id_ + "defaultspace";
  }

  return true;
}

