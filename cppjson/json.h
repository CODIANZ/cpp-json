#if !defined(__cppjson_h_json__)
#define __cppjson_h_json__

#include <string>
#include <unordered_map>
#include <type_traits>
#include <vector>
#include <sstream>

#include "errors.h"

namespace cppjson {
class json
{
public:
  /* js独自の型 */
  struct undefined_type {};
  using array_type = std::vector<json>;
  using object_type = std::unordered_map<std::string, json>;

  /** value_container で保持している型（integral と floating_point はjsではNumber型だが、この世界では別の型として区別する） */
  enum class value_type { integral, floating_point, string, boolean, null, array, object, undefined };

private:
  /** value_container で保持できる型を制限するための判定クラス（set(), get() で使用する） */
  template <typename T> struct is_available_type;
  template <> struct is_available_type<std::string    > { static constexpr bool value = true; };
  template <> struct is_available_type<bool           > { static constexpr bool value = true; };
  template <> struct is_available_type<nullptr_t      > { static constexpr bool value = true; };
  template <> struct is_available_type<array_type     > { static constexpr bool value = true; };
  template <> struct is_available_type<object_type    > { static constexpr bool value = true; };
  template <> struct is_available_type<undefined_type > { static constexpr bool value = true; };
  template <> struct is_available_type<int64_t        > { static constexpr bool value = true; };
  template <> struct is_available_type<double         > { static constexpr bool value = true; };
  template <typename T> struct is_available_type        { static constexpr bool value = false;};

  /** int64_t に変換可能か判定する（int64_tとboolは除外） */
  template <typename T> struct is_integer_compatible {
    static constexpr bool value = std::is_integral<T>::value && (!std::is_same<T, bool>::value) && (!std::is_same<T, int64_t>::value);
  };

  /** double に変換可能か判定する（doubleは除外） */
  template <typename T> struct is_floating_point_compatible {
    static constexpr bool value = std::is_floating_point<T>::value && (!std::is_same<T, double>::value);
  };

  /** value_container の基底クラス */
  class value_container_base {
  protected:
    value_container_base() = default;
  public:
    virtual ~value_container_base() = default;
    virtual value_container_base* clone() const = 0;
    virtual enum value_type value_type() const = 0;
    virtual std::string value_type_string() const = 0;
  };

  /** value_container （型の静的な制約は json クラスの設定・取得系関数で行う） */
  template<typename T>
    class value_container : public value_container_base
  {
  public:
    T value;
    value_container(const T& v) : value(v) {};
    virtual ~value_container() = default;
    virtual value_container_base* clone() const {
      return new value_container<T>(value);
    };
    virtual enum value_type value_type() const {
      auto&& tid = typeid(T);
           if(tid == typeid(std::string    )) return value_type::string;
      else if(tid == typeid(int64_t        )) return value_type::integral;
      else if(tid == typeid(double         )) return value_type::floating_point;
      else if(tid == typeid(bool           )) return value_type::boolean;
      else if(tid == typeid(nullptr_t      )) return value_type::null;
      else if(tid == typeid(array_type     )) return value_type::array;
      else if(tid == typeid(object_type    )) return value_type::object;
      else if(tid == typeid(undefined_type )) return value_type::undefined;
      /** MEMO: 型制約は json 側で行われているのでここにくることは無いはず（静的チェックでコンパイルエラーになっている） */
      bad_type::throw_error();
    }
    virtual std::string value_type_string() const {
      return value_type_string_static();
    }
    static std::string value_type_string_static() {
      auto&& tid = typeid(T);
           if(tid == typeid(std::string    )) return "string";
      else if(tid == typeid(int64_t        )) return "integral";
      else if(tid == typeid(double         )) return "floating_point";
      else if(tid == typeid(bool           )) return "boolean";
      else if(tid == typeid(nullptr_t      )) return "null";
      else if(tid == typeid(array_type     )) return "array";
      else if(tid == typeid(object_type    )) return "object";
      else if(tid == typeid(undefined_type )) return "undefined";
      else {
        std::stringstream ss;
        ss << "invalid type(" << tid.name() << ")";
        return ss.str();
      }
    }
  };

  std::unique_ptr<value_container_base> m_value;

  /** 整数型、浮動小数点の相互型変換を考慮した値取得 */
  template <typename T> T getNumberValue() const {
    auto ivalue = dynamic_cast<value_container<int64_t>*>(m_value.get());
    auto fvalue = dynamic_cast<value_container<double>*>(m_value.get());
    if(ivalue == nullptr && fvalue == nullptr) throw_bad_cast<T>(m_value->value_type_string());
    return ivalue ? static_cast<T>(ivalue->value) : static_cast<T>(fvalue->value);
  }

  /** json の配列による設定の継続処理 */
  template <typename T, typename ...ARGS> void pushValues(array_type& arr, const T& v, ARGS ...args){
    pushTypedValue(arr, v);
    pushValues(arr, std::forward<ARGS>(args)...);
  }

  /** json の配列による設定のゴール地点 */
  void pushValues(array_type& arr) {};

  /** 許容される型のみ arr に追加する。（追加できない型はコンパイルエラー） */
  template <
    typename T,
    std::enable_if_t<
      is_integer_compatible<T>::value |
      is_floating_point_compatible<T>::value |
      is_available_type<T>::value |
      std::is_same<T, const char*>::value
      , bool
    > = true
  > void pushTypedValue(array_type& arr, const T& v) {
    arr.push_back(json(v));
  }

  /** 型変換不能エラー */
  template<typename T> [[noreturn]] static void throw_bad_cast(const std::string& from) {
    auto&& to = value_container<T>::value_type_string_static();
    std::stringstream ss;
    ss << "bad_cast: " << from << " -> " << to;
    throw new bad_cast(ss.str());
  }


protected:


public:
  json() : m_value(new value_container<undefined_type>({})) {}
  json(const json& s) : m_value(s.m_value->clone()) {}
  json(json&& s) : m_value(s.m_value.release()) {}

  /** 整数型（内部では int64_t） */
  template <typename T, std::enable_if_t<is_integer_compatible<T>::value, bool> = true>
  json(const T& v) { m_value.reset(new value_container<int64_t>(static_cast<int64_t>(v))); }

  /** 浮動小数点型（内部では double） */
  template <typename T, std::enable_if_t<is_floating_point_compatible<T>::value, bool> = true>
  json(const T& v) { m_value.reset(new value_container<double>(static_cast<double>(v))); }

  /** その他の許容可能な型 */
  template <typename T, std::enable_if_t<is_available_type<T>::value, bool> = true>
  json(const T& v) { m_value.reset(new value_container<T>(v)); }

  /** C文字列を受け入れる（内部では std::string） */
  json(const char* v) { m_value.reset(new value_container<std::string>(v)); }

  /** object型の initialize_list で構築*/
  json(const std::initializer_list<object_type::value_type>& list) {
    m_value.reset(new value_container<object_type>(list));
  }

  /**
   * json の配列は許容可能な型が混在しているため、 initializer_list は使用せず可変引数テンプレートで逐次処理を行う。
   * 可変引数テンプレートでは、各引数の型チェックが行えないため pushTypedValue() にて静的チェックを行う。
   **/
  template <typename ...ARGS> json(ARGS ...args) {
    m_value.reset(new value_container<array_type>({}));
    auto& arr = get<array_type>();
    pushValues(arr, std::forward<ARGS>(args)...);
  }

  ~json() = default;


  /************** 設定（関数） ***************/
  void set(const json& j) { m_value.reset(j.m_value->clone()); }
  void set(json&& j) { m_value.reset(j.m_value.release()); }


  /************** 設定（代入） ***************/
  json& operator =(const json& j) { set(j); return *this; }
  json& operator =(json&& j) { set(j); return *this; }


  /************** 取得 ***************/
  /** 整数型（int_64tからの型変換を許容するため参照ではなく値のコピーを返却する点に注意） */
  template <typename T, std::enable_if_t<is_integer_compatible<T>::value, bool> = true>
  const T get() const {
    return getNumberValue<T>();
  }
  /** 浮動小数点型（doubleからの型変換を許容するため参照ではなく値のコピーを返却する点に注意） */
  template <typename T, std::enable_if_t<is_floating_point_compatible<T>::value, bool> = true>
  const T get() const {
    return getNumberValue<T>();
  }
  /** その他の許容可能な型（const） */
  template <typename T, std::enable_if_t<is_available_type<T>::value, bool> = true>
  const T& get() const {
    auto avalue = dynamic_cast<value_container<T>*>(m_value.get());
    if(avalue == nullptr) throw_bad_cast<T>(m_value->value_type_string());
    return avalue->value;
  }
  /** その他の許容可能な型（非const） */
  template <typename T, std::enable_if_t<is_available_type<T>::value, bool> = true>
  T& get() {
    auto avalue = dynamic_cast<value_container<T>*>(m_value.get());
    if(avalue == nullptr) throw_bad_cast<T>(m_value->value_type_string());
    return avalue->value;
  }

  /************** メモリ管理 ***************/
  /** 自身を複製する（deep copy） */
  json clone() const {
    json j;
    j.m_value.reset(m_value->clone());
    return j;
  }

  /** 保持している値を開放し、開放された値を返却する。 json は undefined となる */
  template <typename T, std::enable_if_t<is_available_type<T>::value, bool> = true>
  void release(std::unique_ptr<T>& value) {
    value.reset(m_value.release());
    m_value.reset(new value_container<undefined_type>({}));
  }


  /************** 状態・属性 ***************/
  /** 値の型を取得 */
  const value_type value_type() const { return m_value->value_type(); }

  /* undefined, null 確認用関数 */
  bool is_undefined() const         { return value_type() == value_type::undefined; }
  bool is_null() const              { return value_type() == value_type::null; }
  bool is_null_or_undefined() const { return is_undefined() || is_null(); }

  operator bool() const { return is_null_or_undefined(); }

  /************** プロパティ検索 ***************/
  /** object型に対する値の検索（const） */
  const json* find(const std::string& path, const char separator = '.') const {
    if(value_type() != value_type::object) return nullptr;
    const auto pos = path.find(separator);
    auto&& obj = get<object_type>();
    if(pos == std::string::npos){
      auto it = obj.find(path);
      return it != obj.end() ? &it->second : nullptr;
    }
    else{
      const auto left = path.substr(0, pos);
      const auto right = path.substr(pos + 1);
      return obj.find(left)->second.find(right, separator);
    }
  }

  /** object型に対する値の検索（非const） */
  json* find(const std::string& path, const char separator = '.') {
    if(value_type() != value_type::object) return nullptr;
    const auto pos = path.find(separator);
    auto&& obj = get<object_type>();
    if(pos == std::string::npos){
      auto it = obj.find(path);
      return it != obj.end() ? &it->second : nullptr;
    }
    else{
      const auto left = path.substr(0, pos);
      const auto right = path.substr(pos + 1);
      return obj.find(left)->second.find(right, separator);
    }
  }


  /************** operator [] ***************/
  /** object型に対する [] アクセス */

  /** const では見つからない場合、 undefined を返却する */
  const json& operator [](const char * key) const {
    static const json undefined;
    if(value_type() != value_type::object) return undefined;
    auto&& obj = get<object_type>();
    auto it = obj.find(key);
    return it != obj.end() ? it->second : undefined;
  }

  const json& operator [](const std::string& key) const {
    return (*this)[key.c_str()];
  }

  /** 非const では見つからない場合、 object を作成する */
  json& operator [](const char * key) {
    if(value_type() != value_type::object){
      m_value.reset(new value_container<object_type>({}));
    }
    auto&& obj = get<object_type>();
    auto it = obj.find(key);
    if(it != obj.end()) return it->second;
    obj[key] = undefined_type{};
    return obj[key];
  }

  json& operator [](const std::string& key) {
    return (*this)[key.c_str()];
  }

  /** array型に対する [] アクセス */

  /** const では見つからない場合、 undefined を返却する */
  const json& operator [](int index) const {
    static const json undefined;
    if(value_type() != value_type::array) return undefined;
    auto&& arr = get<array_type>();
    return (index < arr.size()) ? arr[index] : undefined;
  }

  /** 非const では見つからない場合、 欠番を nullptr で埋める */
  json& operator [](int index) {
    if(value_type() != value_type::array){
      m_value.reset(new value_container<array_type>({}));
    }
    auto&& arr = get<array_type>();
    if(index < arr.size()) return arr[index];
    arr.resize(index + 1, json(nullptr));
    return arr[index];
  }

};
} /** namespace cppjson */

#endif /* !defined(__cppjson_h_json__) */
