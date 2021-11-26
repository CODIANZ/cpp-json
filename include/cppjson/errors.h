
#if !defined(__cppjson_h_error__)
#define __cppjson_h_error__

#include <exception>
#include <string>

namespace cppjson {

/** 例外オブジェクトの基底クラス */
class error : public std::exception {
private:
  const std::string m_what;
protected:
  error(const std::string& s) : m_what(s) {}
public:
  virtual ~error() = default;
  virtual const char* what() const noexcept { return m_what.c_str(); }
};

/** 取り扱いできない型（value_container内で発生するが、この例外が送出される場合コードのバグである） */
class bad_type : public error {
friend class json;
private:
  bad_type() : error("bad_type") {}
  [[noreturn]] static void throw_error(){
    throw new bad_type();
  }
};

/** 不正な型変換 */
class bad_cast : public error {
friend class json;
private:
  bad_cast(const std::string& s) : error(s) {}
};

/** deserializerのエラー */
class bad_json : public error {
friend class deserializer;
private:
  bad_json(const std::string& s) : error(s) {}
};


} /** namespace cppjson */

#endif /** !defined(__cppjson_h_error__) */