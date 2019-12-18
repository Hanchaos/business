#include "com_xservice.h"
#include "base_data.h"
#include "business_data.h"
#include "capture_data.h"
#include "xiashun_business_data.h"
#include "xiashun_capture_data.h"
#include "com_func.h"
#include "screenshot_data.h"

namespace XService {
class ParseDataFactory {
 public:
  static std::shared_ptr<BaseData> ParseData(std::shared_ptr<PM_DATA> data,
                                             std::string payload) {
    std::shared_ptr<BaseData> producter = nullptr;
    bool is_success;
    Json::Value total_jv;
    Json::Value data_jv;

    //解析原始数据
    is_success = hobotcommon::parse_json(payload, total_jv);
    if (!is_success) return producter;
    if (total_jv.isMember(kdata)) {
      if (total_jv[kdata].isObject()) {
        data_jv = total_jv[kdata];
      } else if (total_jv[kdata].isString()) {
        if (!hobotcommon::parse_json(total_jv[kdata].asString(), data_jv))
          return producter;
      } else {
        return producter;
      }
    } else {
      return producter;
    }
    //解析payload类型
    PayloadType payload_type;
    if (data_jv.isMember(kptype) && data_jv[kptype].isInt()) {
      payload_type = (PayloadType)data_jv[kptype].asInt();
    } else {
      return producter;
    }
    //根据不同的类型生成对应的解析数据类型对象
    switch (payload_type) {
      case XService::PayloadType::Business:
        return std::make_shared<BusinessData>(payload_type, total_jv, data);
        break;
      case XService::PayloadType::Trace:
        return std::make_shared<CaptureData>(payload_type, total_jv, data);
        break;
      case XService::PayloadType::Screenshot:
        return std::make_shared<ScreenshootData>(payload_type, total_jv, data);
        break;
      case XService::PayloadType::xiashunBusiness:
        return std::make_shared<xiashunBusinessData>(payload_type, total_jv, data);
        break;
      case XService::PayloadType::xiashunCapture:
        return std::make_shared<xiashunCaptureData>(payload_type, total_jv, data);
        break;

      default:
        break;
    }
    return producter;
  }
};
}
