
#if !defined(__cppjson_h_object_util__)
#define __cppjson_h_object_util__

#include <string>
#include "json.h"

namespace cppjson {

class object_util {

public:
  template<typename ITER> static json to_json(ITER begin, ITER end) {
    json j = json::object_type();
    auto& obj = j.get<json::object_type>();
    for(ITER it = begin; it != end; it++){
      obj.insert(*it);
    }
    return j;
  }

  template<typename T> static json to_json(const T& src){
    return to_json(std::cbegin(src), std::cend(src));
  }

  static json create(std::function<void(json::object_type&)> f = std::function<void(json::object_type&)>()) {
    json j = json::object_type();
    auto& obj = j.get<json::object_type>();
    if(f){
      f(obj);
    }
    return j;
  }

  static json& edit(json& j, std::function<void(json::object_type&)> f){
    auto& obj = j.get<json::object_type>();
    f(obj);
    return j;
  }
};

} /** namespace cppjson */

#endif /** !defined(__cppjson_h_object_util__) */