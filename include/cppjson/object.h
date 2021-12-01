
#if !defined(__cppjson_h_object__)
#define __cppjson_h_object__

#include "json.h"

namespace cppjson {

class object : public json {
public:
  object(std::initializer_list<json::object_type::value_type>&& list) {
    set(object_type(list));
  }
};

} /** namespace cppjson */

#endif /** !defined(__cppjson_h_object__) */