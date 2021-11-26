
#if !defined(__cppjson_h_array_util__)
#define __cppjson_h_array_util__

#include <string>
#include "json.h"

namespace cppjson {

class array_util {

public:
  template<typename ITER> static json to_json(ITER begin, ITER end) {
    return create([&](json::array_type& arr){
      arr.insert(arr.begin(), begin, end);
    });
  }

  template<typename T> static json to_json(const T& src){
    return to_json(std::cbegin(src), std::cend(src));
  }

  static json create(std::function<void(json::array_type&)> f) {
    json j = create();
    f(j.get<json::array_type>());
    return j;
  }

  static json create() {
    return json::array_type();
  }

  static json& edit(json& j, std::function<void(json::array_type&)> f){
    auto& arr = j.get<json::array_type>();
    f(arr);
    return j;
  }
};

} /** namespace cppjson */

#endif /** !defined(__cppjson_h_array_util__) */