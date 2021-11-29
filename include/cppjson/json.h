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

  /** value_container で保持している型（integral と floating_point はjsではNumber型だが、この世界では別の型として区別する） */
  enum class value_type { integral, floating_point, boolean, null, string, array, object, undefined };

  /** value_container で保持できる型を traits で列挙 */
  template<typename T> struct value_type_traits;
  template<> struct value_type_traits<int64_t       > { static constexpr bool available = true;  static constexpr const char* const name = "integral"  ; static constexpr value_type type = value_type::integral; };
  template<> struct value_type_traits<double        > { static constexpr bool available = true;  static constexpr const char* const name = "double"    ; static constexpr value_type type = value_type::floating_point; };
  template<> struct value_type_traits<bool          > { static constexpr bool available = true;  static constexpr const char* const name = "bool"      ; static constexpr value_type type = value_type::boolean; };
  template<> struct value_type_traits<nullptr_t     > { static constexpr bool available = true;  static constexpr const char* const name = "null"      ; static constexpr value_type type = value_type::null; };
  template<> struct value_type_traits<std::string   > { static constexpr bool available = true;  static constexpr const char* const name = "string"    ; static constexpr value_type type = value_type::string; };
  template<> struct value_type_traits<array_type    > { static constexpr bool available = true;  static constexpr const char* const name = "array"     ; static constexpr value_type type = value_type::array; };
  template<> struct value_type_traits<object_type   > { static constexpr bool available = true;  static constexpr const char* const name = "object"    ; static constexpr value_type type = value_type::object; };
  template<> struct value_type_traits<undefined_type> { static constexpr bool available = false; static constexpr const char* const name = "undefined" ; static constexpr value_type type = value_type::undefined; };
  template<typename T> struct value_type_traits       { static constexpr bool available = false; };

  /** shared pointer 化 */
  using sp = std::shared_ptr<json>;
  sp to_shared() { return std::make_shared<json>(std::move(*this)); }

private:
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

  /**
   * 値を保有するクラス
   * class インスタンスはポインタで保有、その他は実体を保有する。
   **/
  class value_container {
  private:
    union content {
      int64_t       _integer;
      double        _floating_point;
      bool          _boolean;
      nullptr_t     _null;
      std::string*  _string_ptr;
      array_type*   _array_ptr;
      object_type*  _object_ptr;
    };
    content         m_content;
    value_type      m_value_type;
    const char*     m_value_type_string;

    /** 内包する値の解放（classインスタンスは削除するが、それ以外は何もしない） */
    void destruct_value() {
      switch(value_type()){
        case value_type::string:  { delete m_content._string_ptr; break; }
        case value_type::array:   { delete m_content._array_ptr; break; }
        case value_type::object:  { delete m_content._object_ptr; break; }
        default: { break; } /** class インスタンスでなければ何もしない */
      }
    }

    /**
     * 型をundefinedにする。
     * ここではclassインスタンスの削除は行わない点に注意。
     **/
    void reset_to_undefined(){
      m_value_type        = value_type_traits<undefined_type>::type;
      m_value_type_string = value_type_traits<undefined_type>::name;
    }

  public:
    value_container() {
      reset_to_undefined();
    }

    value_container(const value_container& src): m_value_type(value_type::undefined) {
      *this = src;
    }

    value_container(value_container&& src): m_value_type(value_type::undefined) {
      *this = std::move(src);
    }

    template <typename T, std::enable_if_t<value_type_traits<T>::available, bool> = true>
    value_container(T value): m_value_type(value_type::undefined) {
      set(std::forward<T>(value));
    }

    ~value_container() { destruct_value(); }

    value_container& operator = (const value_container& src){
      destruct_value();
      auto clone = src.clone();     /** コピーを生成 */
      m_content = clone.m_content;
      m_value_type = clone.m_value_type;
      m_value_type_string = clone.m_value_type_string;
      clone.reset_to_undefined(); /** 複製した側（clone）でclassインスタンスが破棄されないよう reset しておく */
      return *this;
    }

    value_container& operator = (value_container&& src){
      destruct_value();
      m_content = src.m_content;
      m_value_type = src.m_value_type;
      m_value_type_string = src.m_value_type_string;
      src.reset_to_undefined(); /** src側でclassインスタンスが破棄されないよう reset しておく */
      return *this;
    }

    value_container clone() const {
      switch(value_type()){
        case value_type::integral:        { return value_container(m_content._integer); }
        case value_type::floating_point:  { return value_container(m_content._floating_point); }
        case value_type::boolean:         { return value_container(m_content._boolean); }
        case value_type::null:            { return value_container(m_content._null); }
        case value_type::string:          { return value_container(*m_content._string_ptr); }
        case value_type::array:           { return value_container(*m_content._array_ptr); }
        case value_type::object:          { return value_container(*m_content._object_ptr); }
        default: /** undefined */         { return value_container(); }
      }
    }

    const value_type value_type() const { return m_value_type; }
    const char* value_type_string() const { return m_value_type_string; }

    template <typename T, std::enable_if_t<value_type_traits<T>::available && !std::is_class<T>::value, bool> = true>
    const T& get() const { return *reinterpret_cast<const T*>(&m_content); }

    template <typename T, std::enable_if_t<value_type_traits<T>::available && std::is_class<T>::value, bool> = true>
    const T& get() const { return **reinterpret_cast<T* const *>(&m_content); }

    template <typename T, std::enable_if_t<value_type_traits<T>::available && !std::is_class<T>::value, bool> = true>
    T& get() { return *reinterpret_cast<T*>(&m_content); }

    template <typename T, std::enable_if_t<value_type_traits<T>::available && std::is_class<T>::value, bool> = true>
    T& get() { return **reinterpret_cast<T**>(&m_content); }

    template <typename T, std::enable_if_t<value_type_traits<T>::available && !std::is_class<T>::value, bool> = true>
    void set(T value) {
      destruct_value();
      if(m_value_type != value_type_traits<T>::type){
        m_value_type = value_type_traits<T>::type;
        m_value_type_string = value_type_traits<T>::name;
      }
      using TT = typename std::remove_reference<T>::type;
      *reinterpret_cast<TT*>(&m_content) = TT(std::forward<T>(value));
    }

    template <typename T, std::enable_if_t<value_type_traits<T>::available && std::is_class<T>::value, bool> = true>
    void set(T value) {
      destruct_value();
      if(m_value_type != value_type_traits<T>::type){
        m_value_type = value_type_traits<T>::type;
        m_value_type_string = value_type_traits<T>::name;
      }
      using TT = typename std::remove_reference<T>::type;
      *reinterpret_cast<TT**>(&m_content) = new TT(std::forward<T>(value));
    }
  };

  /** json で保持する唯一の値 */
  value_container m_value;

  /** 整数型、浮動小数点の相互型変換を考慮した値取得 */
  template <typename T> T getNumberValue() const {
    if(m_value.value_type() == value_type::integral){
      return static_cast<T>(m_value.get<int64_t>());
    }
    else if(m_value.value_type() == value_type::floating_point){
      return static_cast<T>(m_value.get<double>());
    }
    else{
      throw_bad_cast<T>(m_value.value_type_string());
    }
  }

  /** json の配列による設定の継続処理 */
  template <typename T, typename ...ARGS> void pushValues(array_type& arr, T v, ARGS ...args){
    pushTypedValue(arr, std::forward<T>(v));
    pushValues(arr, std::forward<ARGS>(args)...);
  }

  /** json の配列による設定のゴール地点 */
  void pushValues(array_type& arr) {};

  /** 許容される型のみ arr に追加（追加できない型はコンパイルエラー） */
  template <
    typename T,
    std::enable_if_t<
      is_number_type<T>::value ||
      value_type_traits<T>::available ||
      std::is_same<T, const char*>::value ||
      std::is_same<T, json>::value
      , bool
    > = true
  > void pushTypedValue(array_type& arr, T v) {
    arr.push_back(std::forward<T>(v));
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
    value_type_traits<T>::available && (!is_number_type<T>::value)
  , bool> = true>
  json(T v) : m_value(std::forward<T>(v)) {}

  /** C文字列を受け入れる（内部では std::string） */
  json(const char* v) : m_value(std::string(v)) {}

  /** object型の initialize_list で構築*/
  json(std::initializer_list<object_type::value_type>&& list) : m_value(object_type(list)) {}

  /**
   * json の配列は許容可能な型が混在しているため、 initializer_list は使用せず可変引数テンプレートで再帰処理を行う。
   * 可変引数テンプレートでは、各引数の型チェックが行えないため pushTypedValue() にて静的チェックを行う。
   * （まぁ、この関数内で static_asster 使っても良いかもだけど）
   **/
  template <typename ...ARGS> json(ARGS ...args) : m_value(array_type()) {
    auto& arr = get<array_type>();
    pushValues(arr, std::forward<ARGS>(args)...);
  }

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
  /** 整数型（int_64tからの型変換を許容するため参照ではなく値のコピーを返却する点に注意） */
  template <typename T, std::enable_if_t<is_integer_compatible<T>::value, bool> = true>
  const T get() const {
    if(is_undefined()) value_is_undefined::throw_error();
    return getNumberValue<T>();
  }
  /** 浮動小数点型（doubleからの型変換を許容するため参照ではなく値のコピーを返却する点に注意） */
  template <typename T, std::enable_if_t<is_floating_point_compatible<T>::value, bool> = true>
  const T get() const {
    if(is_undefined()) value_is_undefined::throw_error();
    return getNumberValue<T>();
  }
  /** その他の許容可能な型（const） */
  template <typename T, std::enable_if_t<
    value_type_traits<T>::available && (!is_number_type<T>::value)
  , bool> = true>
  const T& get() const {
    if(is_undefined()) value_is_undefined::throw_error();
    if(value_type() != value_type_traits<T>::type){
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
    if(value_type() != value_type_traits<T>::type){
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
  const value_type value_type() const { return m_value.value_type(); }

  /* undefined, null 確認用関数 */
  bool is_undefined() const         { return value_type() == value_type::undefined; }
  bool is_null() const              { return value_type() == value_type::null; }
  bool is_null_or_undefined() const { return is_undefined() || is_null(); }

  /* 値が存在するのか判定 */
  operator bool() const { return is_null_or_undefined(); }

  /** T で取得可能か判定する */
  template<typename T, std::enable_if_t<is_number_type<T>::value, bool> = true>
  bool acquirable() const {
    auto&& self_type = value_type();
    return self_type == value_type::integral || self_type == value_type::floating_point;
  }

  template<typename T, std::enable_if_t<value_type_traits<T>::available && !is_number_type<T>::value, bool> = true>
  bool acquirable() const {
    return value_type() == value_type_traits<T>::type;
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
    if(value_type() != value_type::array) return undefined();
    auto&& arr = get<array_type>();
    return (index < arr.size()) ? arr[index] : undefined();
  }

  /** 非const では見つからない場合、 欠番を nullptr で埋める */
  json& operator [](int index) {
    if(value_type() != value_type::array){
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
