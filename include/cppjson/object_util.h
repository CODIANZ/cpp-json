
#if !defined(__cppjson_h_object_util__)
#define __cppjson_h_object_util__

#include <string>
#include "json.h"

namespace cppjson {

class object_util {

public:
  template<typename ITER> static json to_json(ITER begin, ITER end) {
    return create([&](json::object_type& obj){
      obj.insert(begin, end);
    });
  }

  template<typename T> static json to_json(const T& src){
    return to_json(std::cbegin(src), std::cend(src));
  }

  static json create(std::function<void(json::object_type&)> f) {
    json j = create();
    f(j.get<json::object_type>());
    return j;
  }

  static json create() {
    return json::object_type();
  }

  static json& edit(json& j, std::function<void(json::object_type&)> f){
    auto& obj = j.get<json::object_type>();
    f(obj);
    return j;
  }
};

} /** namespace cppjson */

#endif /** !defined(__cppjson_h_object_util__) */