#if !defined(__cppjson_h_path_util__)
#define __cppjson_h_path_util__

#include "json.h"
#include <iostream>

namespace cppjson {

class path_util
{
public:

  /** path に従って object を生成して、最後に値を設定する。 */
  static json create(const std::string& path, const json& value = json(), const char separator = '.') {
    const auto pos = path.find(separator);
    if(pos == std::string::npos){
      return {{path, value}};
    }
    else{
      const auto left = path.substr(0, pos);
      const auto right = path.substr(pos + 1);
      return {{left, create(right, value, separator)}};
    }
  }

  /** path に従って値を取得する。存在しない場合は nullptr を返却する。 */
  static const json* find(const json& j, const std::string& path, const char separator = '.') {
    if(j.value_type() != json::value_type::object) return nullptr;
    auto&& obj = j.get<json::object_type>();
    const auto pos = path.find(separator);
    if(pos == std::string::npos){
      auto it = obj.find(path);
      return it != obj.end() ? &it->second : nullptr;
    }
    else{
      const auto left = path.substr(0, pos);
      const auto right = path.substr(pos + 1);
      auto it = obj.find(left);
      return it != obj.end() ? find(it->second, right, separator) : nullptr;
    }
  }

  /** path に従って値を取得する。存在しない場合は nullptr を返却する。 */
  static json* find(json& j, const std::string& path, const char separator = '.') {
    if(j.value_type() != json::value_type::object) return nullptr;
    auto&& obj = j.get<json::object_type>();
    const auto pos = path.find(separator);
    if(pos == std::string::npos){
      auto it = obj.find(path);
      return it != obj.end() ? &it->second : nullptr;
    }
    else{
      const auto left = path.substr(0, pos);
      const auto right = path.substr(pos + 1);
      auto it = obj.find(left);
      return it != obj.end() ? find(it->second, right, separator) : nullptr;
    }
  }

  /** path に従って値を取得する。存在しない or 非object の場合は作成を行う。object の場合には値を追加する。 */
  static void put(json& j, const std::string& path, const json& value, const char separator = '.') {
    if(j.value_type() != json::value_type::object){
      j.set(json::object_type());
    }
    auto&& obj = j.get<json::object_type>();
    const auto pos = path.find(separator);
    if(pos == std::string::npos){
      obj[path] = value;
    }
    else{
      const auto left = path.substr(0, pos);
      const auto right = path.substr(pos + 1);
      auto it = obj.find(left);
      if(it != obj.end()){
        put(it->second, right, value, separator);
      }
      else{
        obj[left] = create(right, value, separator);
      }
    }
  }
};

} /** namespace cppjson */

#endif /** !defined(__cppjson_h_path_util__) */