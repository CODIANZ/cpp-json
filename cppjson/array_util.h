
#if !defined(__cppjson_h_array_util__)
#define __cppjson_h_array_util__

#include <string>
#include "json.h"

namespace cppjson {

class array_util {

public:
  template<typename ITER> static json to_json(ITER begin, ITER end) {
    json j = json::array_type();
    auto& arr = j.get<json::array_type>();
    for(ITER it = begin; it != end; it++){
      arr.push_back(*it);
    }
    return j;
  }

  template<typename T> static json to_json(const T& src){
    return to_json(std::cbegin(src), std::cend(src));
  }

  static json create(std::function<void(json::array_type&)> f = std::function<void(json::array_type&)>()) {
    json j = json::array_type();
    auto& arr = j.get<json::array_type>();
    if(f){
      f(arr);
    }
    return j;
  }

  static json& edit(json& j, std::function<void(json::array_type&)> f){
    auto& arr = j.get<json::array_type>();
    f(arr);
    return j;
  }
};

} /** namespace cppjson */

#endif /** !defined(__cppjson_h_array_util__) */