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
private:
  /* js独自の型（非公開） */
  struct undefined_type {};

public:
  /* js独自の型（公開） */
  using array_type = std::vector<json>;
  using object_type = std::unordered_map<std::string, json>;

  /** value_container で保持している型（integral と floating_point はjsではNumber型だが、この世界では別の型として区別する） */
  enum class value_type { integral, floating_point, string, boolean, null, array, object, undefined };

  /** value_container で保持できる型を traits で列挙 */
  template<typename T> struct value_type_traits;
  template<> struct value_type_traits<std::string   > { static constexpr bool available = true; static constexpr const char* const name = "string"    ; static constexpr value_type type = value_type::string; };
  template<> struct value_type_traits<int64_t       > { static constexpr bool available = true; static constexpr const char* const name = "integral"  ; static constexpr value_type type = value_type::integral; };
  template<> struct value_type_traits<double        > { static constexpr bool available = true; static constexpr const char* const name = "double"    ; static constexpr value_type type = value_type::floating_point; };
  template<> struct value_type_traits<bool          > { static constexpr bool available = true; static constexpr const char* const name = "bool"      ; static constexpr value_type type = value_type::boolean; };
  template<> struct value_type_traits<nullptr_t     > { static constexpr bool available = true; static constexpr const char* const name = "null"      ; static constexpr value_type type = value_type::null; };
  template<> struct value_type_traits<array_type    > { static constexpr bool available = true; static constexpr const char* const name = "array"     ; static constexpr value_type type = value_type::array; };
  template<> struct value_type_traits<object_type   > { static constexpr bool available = true; static constexpr const char* const name = "object"    ; static constexpr value_type type = value_type::object; };
  template<> struct value_type_traits<undefined_type> { static constexpr bool available = true; static constexpr const char* const name = "undefined" ; static constexpr value_type type = value_type::undefined; };
  template<typename T> struct value_type_traits       { static constexpr bool available = false; };

  /** shared pointer 化 */
  using sp = std::shared_ptr<json>;
  sp to_shared() { return std::make_shared<json>(std::move(*this)); }

private:
  /** int64_t に変換可能か判定する */
  template <typename T> struct is_integer_compatible {
    static constexpr bool value = std::is_integral<T>::value && (!std::is_same<T, bool>::value);
  };

  /** double に変換可能か判定する（doubleは除外） */
  template <typename T> struct is_floating_point_compatible {
    static constexpr bool value = std::is_floating_point<T>::value;
  };

  /** Number型（integer or floating point）を判定判定 */
  template <typename T> struct is_number_type {
    static constexpr bool value = is_integer_compatible<T>::value || is_floating_point_compatible<T>::value;
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
    value_container(T&& v) : value(std::move(v)) {};
    virtual ~value_container() = default;
    virtual value_container_base* clone() const {
      return new value_container<T>(value);
    };
    virtual enum value_type value_type() const {
      return value_type_traits<T>::type;
    }
    virtual std::string value_type_string() const {
      return value_type_traits<T>::name;
    }
  };

  /** json で保持する唯一の値 */
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

  /** 許容される型のみ arr に追加（追加できない型はコンパイルエラー） */
  template <
    typename T,
    std::enable_if_t<
      is_integer_compatible<T>::value ||
      is_floating_point_compatible<T>::value ||
      value_type_traits<T>::available ||
      std::is_same<T, const char*>::value
      , bool
    > = true
  > void pushTypedValue(array_type& arr, const T& v) {
    arr.push_back(v);
  }

  /** 型変換不能エラー */
  template<typename T, std::enable_if_t<value_type_traits<T>::available, bool> = true>
  [[noreturn]] static void throw_bad_cast(const std::string& from) {
    std::stringstream ss;
    ss << "bad_cast: " << from << " -> " << value_type_traits<T>::name;
    throw new bad_cast(ss.str());
  }
  template<typename T, std::enable_if_t<!value_type_traits<T>::available, bool> = true>
  [[noreturn]] static void throw_bad_cast(const std::string& from) {
    std::stringstream ss;
    ss << "bad_cast: " << from << " -> " << typeid(T).name();
    throw new bad_cast(ss.str());
  }
  
  /** const json& で undefined を返却する場合のインスタンス保有をする */
  static const json& undefined() {
    static const json undefined_const;
    return undefined_const;
  }

protected:


public:
  /************** インスタンス生成・破棄 ***************/

  /** デフォルト・コピー・ムーブ */
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
  template <typename T, std::enable_if_t<
    value_type_traits<T>::available && (!is_number_type<T>::value)
  , bool> = true>
  json(const T& v) { m_value.reset(new value_container<T>(v)); }

  /** その他の許容可能な型（右辺値） */
  template <typename T, std::enable_if_t<
    value_type_traits<T>::available && (!is_number_type<T>::value)
  , bool> = true>
  json(T&& v) { m_value.reset(new value_container<T>(std::move(v))); }

  /** C文字列を受け入れる（内部では std::string） */
  json(const char* v) { m_value.reset(new value_container<std::string>(v)); }

  /** object型の initialize_list で構築*/
  json(const std::initializer_list<object_type::value_type>& list) {
    m_value.reset(new value_container<object_type>(list));
  }

  /**
   * json の配列は許容可能な型が混在しているため、 initializer_list は使用せず可変引数テンプレートで再帰処理を行う。
   * 可変引数テンプレートでは、各引数の型チェックが行えないため pushTypedValue() にて静的チェックを行う。
   * （まぁ、この関数内で static_asster 使っても良いかもだけど）
   **/
  template <typename ...ARGS> json(ARGS ...args) {
    m_value.reset(new value_container<array_type>({}));
    auto& arr = get<array_type>();
    pushValues(arr, std::forward<ARGS>(args)...);
  }

  /** デストラクタ */
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
  template <typename T, std::enable_if_t<
    value_type_traits<T>::available && (!is_number_type<T>::value)
  , bool> = true>
  const T& get() const {
    auto avalue = dynamic_cast<value_container<T>*>(m_value.get());
    if(avalue == nullptr) throw_bad_cast<T>(m_value->value_type_string());
    return avalue->value;
  }
  /** その他の許容可能な型（非const） */
  template <typename T, std::enable_if_t<
    value_type_traits<T>::available && (!is_number_type<T>::value)
  , bool> = true>
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
  template <typename T, std::enable_if_t<value_type_traits<T>::available, bool> = true>
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

  /** T で取得可能か判定する */
  template<typename T> bool acquirable() const {
    if(is_number_type<T>::value){
      auto&& self_type = value_type();
      return self_type == value_type::integral || self_type == value_type::floating_point;
    }
    else if(value_type_traits<T>::available){
      return dynamic_cast<value_container<T>*>(m_value.get()) != nullptr;
    }
    else {
      return false;
    }
  }


  /************** operator [] ***************/
  /** object型に対する [] アクセス */

  /** const では見つからない場合、 undefined を返却する */
  const json& operator [](const char * key) const {
    if(value_type() != value_type::object) return undefined();
    auto&& obj = get<object_type>();
    auto it = obj.find(key);
    return it != obj.end() ? it->second : undefined();
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
    if(value_type() != value_type::array) return undefined();
    auto&& arr = get<array_type>();
    return (index < arr.size()) ? arr[index] : undefined();
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
