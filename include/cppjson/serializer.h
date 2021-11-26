#if !defined(__cppjson_h_serializer__)
#define __cppjson_h_serializer__

#include "json.h"
#include <ostream>
#include <iomanip>

namespace cppjson {
class serializer {
private:
  const json& m_json;
  const std::string m_indent;

  void insertIndent(std::ostream& os, int level) const {
    if(level > 0 && m_indent.size() > 0){
      for(auto i = 0; i < level; i++){
        os << m_indent;
      }
    }
  }

  void insertNewLine(std::ostream& os) const {
    if(m_indent.size() > 0) os << std::endl;
  }

 void escape(std::ostream& os, const std::string& src) const {
    for(auto c : src){
      switch(c){
        case '"' : { os << "\\\"";  break; }
        case '\b': { os << "\\b";  break; }
        case '\f': { os << "\\f";  break; }
        case '\n': { os << "\\n";  break; }
        case '\r': { os << "\\r";  break; }
        case '\t': { os << "\\t";  break; }
        case '\\': { os << "\\\\"; break; }
        case '/' : { os << "\\/";  break; }
        default:
        {
          if((0x00 <= c) && (c <= 0x1F)){
            os << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
          }
          else{
            os << c;
          }
          break;
        }
      }
    }
  }

  void proceed(std::ostream& os, const json& j, int level) const {
    switch(j.value_type()) {
      case json::value_type::integral: {
        os << j.get<int64_t>();
        break;
      }
      case json::value_type::floating_point: {
        os << j.get<double>();
        break;
      }
      case json::value_type::string: {
        os << "\"";
        escape(os, j.get<std::string>());
        os << "\"";
        break;
      }
      case json::value_type::boolean: {
        os << (j.get<bool>() ? "true" : "false");
        break;
      }
      case json::value_type::null: {
        os << "null";
        break;
      }
      case json::value_type::array: {
        os << "[";
        insertNewLine(os);
        const auto& arr = j.get<json::array_type>();
        for(auto it = arr.begin(); it != arr.end(); it++){
          if(it != arr.begin()){
            os << ",";
            insertNewLine(os);
          }
          insertIndent(os, level + 1);
          proceed(os, *it, level + 1);
        }
        insertNewLine(os);
        insertIndent(os, level);
        os << "]";
        break;
      }
      case json::value_type::object: {
        os << "{";
        insertNewLine(os);
        const auto& obj = j.get<json::object_type>();
        for(auto it = obj.begin(); it != obj.end(); it++){
          if(it != obj.begin()){
            os << ",";
            insertNewLine(os);
          }
          insertIndent(os, level + 1);
          os << "\"";
          escape(os, it->first);
          os << "\"" << ":";
          if(m_indent.size() > 0) os << " ";
          proceed(os, it->second, level + 1);
        }
        insertNewLine(os);
        insertIndent(os, level);
        os << "}";
        break;
      }
      case json::value_type::undefined: {
        /**
         * json では undefined を表現できない。
         * また、JSON.strigify()では undefined は null となるため仕様を合わせた
         **/
        os << "null";
        break;
      }
    }
  }


public:
  serializer(const json& j, const std::string& indent = std::string(""))
    : m_json(j), m_indent(indent) {}

  void execute(std::ostream& os) const{
    proceed(os, m_json, 0);
  }

  std::string execute() const {
    std::stringstream ss;
    execute(ss);
    return ss.str();
  }
};
} /** namespace cppjson */
#endif /* !defined(__cppjson_h_serializer__) */