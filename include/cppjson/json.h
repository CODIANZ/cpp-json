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
  struct undefined_type {}; /** 内部でのみ使用 */

public:
  /* js独自の型（公開） */
  using array_type = std::vector<json>;
  using object_type = std::unordered_map<std::string, json>;

  /** value_container で保持している型を値として取り扱うための ID （integral と floating_point はjsではNumber型だが、この世界では別の型として区別する） */
  enum class value_type_id { integral, floating_point, boolean, null, string, array, object, undefined };

  /** value_type_traits で有効な型の特性（使用可否・型・型ID）を保有する */
  template<typename T, value_type_id VALUE_TYPE_ID> struct traits_holder {
    static constexpr bool           available = true;
    using                           type = T;
    static constexpr value_type_id  value_type_id = VALUE_TYPE_ID;
  };

  /** value_container で保持できる型を traits で列挙 */
  template<typename T> struct value_type_traits;
  template<> struct value_type_traits<int64_t       > : public traits_holder<int64_t        , value_type_id::integral> {};
  template<> struct value_type_traits<double        > : public traits_holder<double         , value_type_id::floating_point> {};
  template<> struct value_type_traits<bool          > : public traits_holder<bool           , value_type_id::boolean> {};
  template<> struct value_type_traits<nullptr_t     > : public traits_holder<nullptr_t      , value_type_id::null> {};
  template<> struct value_type_traits<std::string   > : public traits_holder<std::string    , value_type_id::string> {};
  template<> struct value_type_traits<array_type    > : public traits_holder<array_type     , value_type_id::array> {};
  template<> struct value_type_traits<object_type   > : public traits_holder<object_type    , value_type_id::object> {};
  template<> struct value_type_traits<undefined_type> : public traits_holder<undefined_type , value_type_id::undefined> {};
  template<typename T> struct value_type_traits       { static constexpr bool available = false; }; /** その他 = 使用できない型 */

  /** T&& を受ける場合、const と 参照を外して value_type_traits を判定する */
  template <typename T> struct pure_value_type_traits : public value_type_traits<
    typename std::remove_const<
      typename std::remove_reference<T>::type
    >::type
  > {};

  /** int64_t に変換可能か判定する */
  template <typename T> struct is_integer_compatible {
    static constexpr bool value = std::is_integral<T>::value && (!std::is_same<T, bool>::value);
  };

  /** double に変換可能か判定する */
  template <typename T> struct is_floating_point_compatible {
    static constexpr bool value = std::is_floating_point<T>::value;
  };

  /** Number型（integer or floating point）か判定する */
  template <typename T> struct is_number_type {
    static constexpr bool value = is_integer_compatible<T>::value || is_floating_point_compatible<T>::value;
  };

  /** shared pointer 化 */
  using sp = std::shared_ptr<json>;
  sp to_shared() { return std::make_shared<json>(std::move(*this)); }

private:
  /** debug での使用を想定 */
  static const char* value_type_string(enum value_type_id value_type_id) { 
    switch(value_type_id) {
      case value_type_id::integral:        { return "integral"; }
      case value_type_id::floating_point:  { return "floating_point"; }
      case value_type_id::boolean:         { return "boolean"; }
      case value_type_id::null:            { return "null"; }
      case value_type_id::string:          { return "string"; }
      case value_type_id::array:           { return "array"; }
      case value_type_id::object:          { return "object"; }
      default: /** undefined */            { return "undefined"; }
    }
  }

  /**
   * 値を保有するクラス
   * class インスタンスはポインタで保有、その他は実体を保有する。
   **/
  class value_container {
  private:
    static constexpr size_t content_size = std::max({
      sizeof(int64_t),
      sizeof(double),
      sizeof(bool),
      sizeof(nullptr_t),
      sizeof(std::string),
      sizeof(array_type),
      sizeof(object_type)
    });
    uint8_t         m_content[content_size];
    value_type_id   m_value_type_id;

    template<typename T> T* ptr_as() {
      return reinterpret_cast<T*>(m_content);
    }

    template<typename T> const T* ptr_as() const {
      return reinterpret_cast<const T*>(m_content);
    }

    /** 内包する値の解放（classインスタンスは削除するが、それ以外は何もしない） */
    void destruct_value() {
      switch(value_type_id()){
        case value_type_id::string:  { ptr_as<std::string>()->~basic_string(); break; }
        case value_type_id::array:   { ptr_as<array_type>()->~vector(); break; }
        case value_type_id::object:  { ptr_as<object_type>()->~unordered_map(); break; }
        default: { break; } /** class インスタンスでなければ何もしない */
      }
      m_value_type_id = value_type_id::undefined;
    }

    /** src から所有権を移転する */
    void move_from(value_container&& src){
      destruct_value();
      switch(src.m_value_type_id){
        case value_type_id::integral:       { *ptr_as<int64_t>()   = *src.ptr_as<int64_t>(); break; }
        case value_type_id::floating_point: { *ptr_as<double>()    = *src.ptr_as<double>(); break; }
        case value_type_id::boolean:        { *ptr_as<bool>()      = *src.ptr_as<bool>(); break; }
        case value_type_id::null:           { *ptr_as<nullptr_t>() = *src.ptr_as<nullptr_t>(); break; }
        case value_type_id::string: { new (m_content) std::string(std::move(*src.ptr_as<std::string>())); break; }
        case value_type_id::array:  { new (m_content) array_type (std::move(*src.ptr_as<array_type>())); break; }
        case value_type_id::object: { new (m_content) object_type(std::move(*src.ptr_as<object_type>())); break; }
        default: { break; } /* undefined */
      }
      m_value_type_id = src.m_value_type_id;
      src.destruct_value();
    }

  public:
    value_container(): m_value_type_id(value_type_id::undefined) {}

    value_container(const value_container& src): m_value_type_id(value_type_id::undefined) {
      *this = src;
    }

    value_container(value_container&& src): m_value_type_id(value_type_id::undefined) {
      *this = std::move(src);
    }

    template <typename T, std::enable_if_t<pure_value_type_traits<T>::available, bool> = true>
    value_container(T&& value): m_value_type_id(value_type_id::undefined) {
      set(std::forward<T>(value));
    }

    ~value_container() { destruct_value(); }

    value_container& operator = (const value_container& src){
      move_from(src.clone());
      return *this;
    }

    value_container& operator = (value_container&& src){
      move_from(std::move(src));
      return *this;
    }

    value_container clone() const {
      switch(value_type_id()){
        case value_type_id::integral:       { return value_container(*ptr_as<int64_t>()); }
        case value_type_id::floating_point: { return value_container(*ptr_as<double>()); }
        case value_type_id::boolean:        { return value_container(*ptr_as<bool>()); }
        case value_type_id::null:           { return value_container(*ptr_as<nullptr_t>()); }
        case value_type_id::string:         { return value_container(*ptr_as<std::string>()); }
        case value_type_id::array:          { return value_container(*ptr_as<array_type>()); }
        case value_type_id::object:         { return value_container(*ptr_as<object_type>()); }
        default: /** undefined */           { return value_container(); }
      }
    }

    const value_type_id value_type_id() const { return m_value_type_id; }

    const char* value_type_string() const {
      return json::value_type_string(value_type_id());
    }

    template <typename T, std::enable_if_t<value_type_traits<T>::available, bool> = true>
    const T& get() const { return *ptr_as<T>(); }

    template <typename T, std::enable_if_t<value_type_traits<T>::available, bool> = true>
    T& get() { return *ptr_as<T>(); }

    template <
      typename T,
      typename PURE_T = typename pure_value_type_traits<T>::type,
      enum value_type_id VALUE_TYPE_ID = pure_value_type_traits<T>::value_type_id,
      std::enable_if_t<!std::is_class<PURE_T>::value, bool> = true
    >
    void set(T&& value) {
      destruct_value();
      m_value_type_id = VALUE_TYPE_ID;
      *ptr_as<PURE_T>() = std::forward<T>(value);
    }

    template <
      typename T,
      typename PURE_T = typename pure_value_type_traits<T>::type,
      enum value_type_id VALUE_TYPE_ID = pure_value_type_traits<T>::value_type_id,
      std::enable_if_t<std::is_class<PURE_T>::value, bool> = true
    >
    void set(T&& value) {
      destruct_value();
      m_value_type_id = VALUE_TYPE_ID;
      new(m_content) PURE_T(std::forward<T>(value));
    }
  };

  /** json で保持する唯一の値 */
  value_container m_value;

  /** 型変換不能エラー */
  template<typename T, std::enable_if_t<value_type_traits<T>::available, bool> = true>
  [[noreturn]] static void throw_bad_cast(const std::string& from) {
    std::stringstream ss;
    ss << "bad_cast: " << from << " -> " << value_type_string(value_type_traits<T>::value_type_id);
    throw bad_cast(ss.str());
  }
  template<typename T, std::enable_if_t<!value_type_traits<T>::available, bool> = true>
  [[noreturn]] static void throw_bad_cast(const std::string& from) {
    std::stringstream ss;
    ss << "bad_cast: " << from << " -> " << typeid(T).name();
    throw bad_cast(ss.str());
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
  json() = default;
  json(const json& s) : m_value(s.m_value) {}
  json(json&& s) : m_value(std::move(s.m_value)) {}

  /** 整数型（内部では int64_t） */
  template <typename T, std::enable_if_t<is_integer_compatible<T>::value, bool> = true>
  json(const T& v) : m_value(static_cast<int64_t>(v)) {}

  /** 浮動小数点型（内部では double） */
  template <typename T, std::enable_if_t<is_floating_point_compatible<T>::value, bool> = true>
  json(const T& v) : m_value(static_cast<double>(v)) {}

  /** その他の許容可能な型 */
  template <typename T, std::enable_if_t<
    pure_value_type_traits<T>::available &&
    (!is_number_type<T>::value)
  , bool> = true>
  json(T&& v) : m_value(std::forward<T>(v)) {}

  /** C文字列を受け入れる（内部では std::string） */
  json(const char* v) : m_value(std::string(v)) {}

  /** object型の initialize_list で構築*/
  json(std::initializer_list<object_type::value_type>&& list) : m_value(object_type(list)) {}

  /** デストラクタ */
  ~json() = default;


  /************** 設定（関数） ***************/
  void set(const json& src) { m_value = src.m_value; }
  void set(json&& j) {
    m_value = std::move(j.m_value);
  }

  /************** 設定（代入） ***************/
  json& operator =(const json& j) { set(j); return *this; }
  json& operator =(json&& j) { set(std::move(j)); return *this; }


  /************** 取得 ***************/
  /** number型（int_64 or doubletからの型変換を許容するため参照ではなく値のコピーを返却する点に注意） */
  template <typename T, std::enable_if_t<is_number_type<T>::value, bool> = true>
  const T get() const {
    if(is_undefined()) value_is_undefined::throw_error();
    if(m_value.value_type_id() == value_type_id::integral){
      return static_cast<T>(m_value.get<int64_t>());
    }
    else if(m_value.value_type_id() == value_type_id::floating_point){
      return static_cast<T>(m_value.get<double>());
    }
    else{
      throw_bad_cast<T>(m_value.value_type_string());
    }
  }
  /** その他の許容可能な型（const） */
  template <typename T, std::enable_if_t<
    value_type_traits<T>::available && (!is_number_type<T>::value)
  , bool> = true>
  const T& get() const {
    if(is_undefined()) value_is_undefined::throw_error();
    if(value_type_id() != value_type_traits<T>::value_type_id){
      throw_bad_cast<T>(m_value.value_type_string());
    }
    return m_value.get<T>();
  }
  /** その他の許容可能な型（非const） */
  template <typename T, std::enable_if_t<
    value_type_traits<T>::available && (!is_number_type<T>::value)
  , bool> = true>
  T& get() {
    if(is_undefined()) value_is_undefined::throw_error();
    if(value_type_id() != value_type_traits<T>::value_type_id){
      throw_bad_cast<T>(m_value.value_type_string());
    }
    return m_value.get<T>();
  }

  /************** メモリ管理 ***************/
  /** 自身を複製する（deep copy） */
  json clone() const {
    json j;
    j.m_value= m_value.clone();
    return j;
  }

  /** 保持している値を開放し、開放された値を返却する。 json は undefined となる。 */
  template <typename T, std::enable_if_t<value_type_traits<T>::available, bool> = true>
  T release() {
    return std::move(m_value.get<T>());
  }


  /************** 状態・属性 ***************/
  /** 値の型を取得 */
  const value_type_id value_type_id() const { return m_value.value_type_id(); }

  /* undefined, null 確認用関数 */
  bool is_undefined() const         { return value_type_id() == value_type_id::undefined; }
  bool is_null() const              { return value_type_id() == value_type_id::null; }
  bool is_null_or_undefined() const { return is_undefined() || is_null(); }

  /* 値が存在するのか判定 */
  operator bool() const { return is_null_or_undefined(); }

  /** T で取得可能か判定する */
  template<typename T, std::enable_if_t<is_number_type<T>::value, bool> = true>
  bool acquirable() const {
    auto&& self_type_id = value_type_id();
    return self_type_id == value_type_id::integral || self_type_id == value_type_id::floating_point;
  }

  template<typename T, std::enable_if_t<value_type_traits<T>::available && !is_number_type<T>::value, bool> = true>
  bool acquirable() const {
    return value_type_id() == value_type_traits<T>::value_type_id;
  }

  /************** operator [] ***************/
  /** object型に対する [] アクセス */

  /** const では見つからない場合、 undefined を返却する */
  const json& operator [](const char * key) const {
    if(value_type_id() != value_type_id::object) return undefined();
    auto&& obj = get<object_type>();
    auto it = obj.find(key);
    return it != obj.end() ? it->second : undefined();
  }

  const json& operator [](const std::string& key) const {
    return (*this)[key.c_str()];
  }

  /** 非const では見つからない場合、 object を作成する */
  json& operator [](const char * key) {
    if(value_type_id() != value_type_id::object){
      m_value.set(object_type());
    }
    auto&& obj = get<object_type>();
    auto it = obj.find(key);
    if(it != obj.end()) return it->second;
    obj[key] = json();
    return obj[key];
  }

  json& operator [](const std::string& key) {
    return (*this)[key.c_str()];
  }

  /** array型に対する [] アクセス */

  /** const では見つからない場合、 undefined を返却する */
  const json& operator [](int index) const {
    if(value_type_id() != value_type_id::array) return undefined();
    auto&& arr = get<array_type>();
    return (index < arr.size()) ? arr[index] : undefined();
  }

  /** 非const では見つからない場合、 欠番を nullptr で埋める */
  json& operator [](int index) {
    if(value_type_id() != value_type_id::array){
      m_value.set(array_type());
    }
    auto&& arr = get<array_type>();
    if(index < arr.size()) return arr[index];
    arr.resize(index + 1, json(nullptr));
    return arr[index];
  }

};
} /** namespace cppjson */

#endif /* !defined(__cppjson_h_json__) */
