#if !defined(__h_json__)
#define __h_json__

#include <string>
#include <unordered_map>
#include <type_traits>
#include <vector>
#include <sstream>

class json
{
public:
  /* js独自の型 */
  struct undefined_type {};
  using array_type = std::vector<json>;
  using object_type = std::unordered_map<std::string, json>;

  /** value_container で保持している型（integral と floating_point はjsではNumber型だが、この世界では別の型として区別する） */
  enum class value_type { integral, floating_point, string, boolean, null, array, object, undefined };

  /** value_container で保持できる型を制限するための判定クラス（setValue(), getValue() で使用する） */
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
      json::bad_type::throw_error();
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
      /** MEMO: 型制約は json 側で行われているのでここにくることは無いはず（静的チェックでコンパイルエラーになっている） */
      json::bad_type::throw_error();
    }
  };

  /** 例外オブジェクト */
  class error : public std::exception {
  private:
    const std::string m_what;
  protected:
    error(const std::string& s) : m_what(s) {}
  public:
    virtual ~error() = default;
    virtual const char* what() const noexcept { return m_what.c_str(); }
  };

  class bad_type : public error {
  private:
    bad_type() : error("bad_type") {}
  public:
    [[noreturn]] static void throw_error(){
      throw new bad_type();
    }
  };

  class bad_cast : public error {
  private:
    bad_cast(const std::string& s) : error(s) {}
  public:
    template<typename T, typename U = T> [[noreturn]] static void throw_error(const std::string& from) {
      auto&& to = value_container<U>::value_type_string_static();
      std::stringstream ss;
      ss << "bad_cast: " << from << " -> " << to;
      throw new bad_cast(ss.str());
    }
  };

private:
  std::unique_ptr<value_container_base> m_value;

  /** 整数型、浮動小数点の相互型変換を考慮した値取得 */
  template <typename T, typename U> T getNumberValue() const {
    auto ivalue = dynamic_cast<value_container<int64_t>*>(m_value.get());
    auto fvalue = dynamic_cast<value_container<double>*>(m_value.get());
    if(ivalue == nullptr && fvalue == nullptr) bad_cast::throw_error<T, U>(m_value->value_type_string());
    return ivalue ? static_cast<T>(ivalue->value) : static_cast<T>(fvalue->value);
  }

  /** value_container_base は型消去されており、生成に型制約が存在しないので、外部公開は行わない */
  json(value_container_base* v) : m_value(v) {};

protected:

public:
  json() : m_value(new value_container<undefined_type>({})) {}
  json(const json& s) : m_value(s.m_value->clone()) {}

  /** 許容される値で構築（型の妥当性は setValue に委譲） */
  template <typename T> json(const T& v) {
    setValue(v);
  }

  /** object型の initialize_list で構築*/
  json(const std::initializer_list<object_type::value_type>& list){
    setValue(list);
  }

  /**
   * json の initializer_list で構築
   * TODO: これが実現できると `json x = {1, true, "123"};` 的な表現が可能になる。
   * しかし、{"aaa", 1} という表現が連想配列なのか、単に配列なのかが不明瞭でコンパイルエラーになる。
   * したがって、現在は保留とする。
   **/
  // json(const std::initializer_list<json>& list) {
  //   setValue(list);
  // }

  /* avalable_type と変換可能な値の initializer_list で構築（型の妥当性は setValue に委譲） */
  template <typename T> json(const std::initializer_list<T>& list) {
    setValue(list);
  }

  ~json() = default;

  /************** 設定（関数） ***************/
  /** 整数型（内部では int64_t） */
  template <typename T, std::enable_if_t<is_integer_compatible<T>::value, bool> = true>
  void setValue(const T& v) { m_value.reset(new value_container<int64_t>(static_cast<int64_t>(v))); }

  /** 浮動小数点型（内部では double） */
  template <typename T, std::enable_if_t<is_floating_point_compatible<T>::value, bool> = true>
  void setValue(const T& v) { m_value.reset(new value_container<double>(static_cast<double>(v))); }

  /** その他の許容可能な型 */
  template <typename T, std::enable_if_t<is_available_type<T>::value, bool> = true>
  void setValue(const T& v) { m_value.reset(new value_container<T>(v)); }

  /** C文字列を受け入れる（内部では std::string） */
  void setValue(const char* v){ m_value.reset(new value_container<std::string>(v)); }

  /** object型の initialize_list */
  void setValue(const std::initializer_list<object_type::value_type>& list){
    m_value.reset(new value_container<object_type>(list));
  }

  /** json の initializer_list */
  // void setValue(const std::initializer_list<json>& list) {  
  //   m_value.reset(new value_container<array_type>(list));
  // }

  /* avalable_type と変換可能な値の initializer_list */
  template <typename T, std::enable_if_t<
    is_available_type<T>::value ||
    is_integer_compatible<T>::value ||
    is_floating_point_compatible<T>::value ||
    std::is_same<T, const char*>::value
    , bool> = true>
  void setValue(const std::initializer_list<T>& list) {  
    auto arr = std::unique_ptr<value_container<array_type>>(new value_container<array_type>({}));
    auto it = list.begin();
    for(auto it = list.begin(); it != list.end(); it++){
      arr->value.push_back(json(*it));
    }
    m_value.reset(arr.release());
  }


  /************** 設定（代入） ***************/
  /** 整数型（内部では int64_t） */
  template <typename T, std::enable_if_t<is_integer_compatible<T>::value, bool> = true>
  json& operator =(const T& v) { setValue(v); return *this; }

  /** 浮動小数点型（内部では double） */
  template <typename T, std::enable_if_t<is_floating_point_compatible<T>::value, bool> = true>
  json& operator =(const T& v) { setValue(v); return *this; }

  /** その他の許容可能な型 */
  template <typename T, std::enable_if_t<is_available_type<T>::value, bool> = true>
  json& operator =(const T& v) { setValue(v); return *this; }

  /** C文字列を受け入れる（内部では std::string） */
  json& operator =(const char* v) { setValue(v); return *this; }

  /** object型の initialize_list */
  json& operator =(const std::initializer_list<object_type::value_type>& list){
    setValue(list);
    return *this;
  }

  /** json の initializer_list */
  // json& operator =(const std::initializer_list<json>& list) {  
  //   setValue(list);
  //   return *this;
  // }

  /* avalable_type と変換可能な数値の initializer_list */
  template <typename T, std::enable_if_t<
    is_available_type<T>::value ||
    is_integer_compatible<T>::value ||
    is_floating_point_compatible<T>::value ||
    std::is_same<T, const char*>::value
    , bool> = true>
  json& operator =(const std::initializer_list<T>& list) {  
    setValue(list);
    return *this;
  }

  /************** 取得 ***************/
  /** 整数型（int_64tからの型変換を許容するため参照ではなく値のコピーを返却する点に注意） */
  template <typename T, std::enable_if_t<is_integer_compatible<T>::value, bool> = true>
  const T getValue() const {
    return getNumberValue<T, int64_t>();
  }
  /** 浮動小数点型（doubleからの型変換を許容するため参照ではなく値のコピーを返却する点に注意） */
  template <typename T, std::enable_if_t<is_floating_point_compatible<T>::value, bool> = true>
  const T getValue() const {
    return getNumberValue<T, double>();
  }
  /** その他の許容可能な型（const） */
  template <typename T, std::enable_if_t<is_available_type<T>::value, bool> = true>
  const T& getValue() const {
    auto avalue = dynamic_cast<value_container<T>*>(m_value.get());
    if(avalue == nullptr) bad_cast::throw_error<T>(m_value->value_type_string());
    return avalue->value;
  }
  /** その他の許容可能な型（非const） */
  template <typename T, std::enable_if_t<is_available_type<T>::value, bool> = true>
  T& getValue() {
    auto avalue = dynamic_cast<value_container<T>*>(m_value.get());
    if(avalue == nullptr) bad_cast::throw_error<T>(m_value->value_type_string());
    return avalue->value;
  }


  /************** メモリ管理 ***************/
  /** 自身を複製する（deep copy） */
  json clone() const {
    return json(m_value->clone());
  }

  /** 保持している値を開放し、開放された値を返却する。 json は undefined となる */
  template <typename T, std::enable_if_t<is_available_type<T>::value, bool> = true>
  void release(std::unique_ptr<T>& value) {
    value.reset(m_value.release());
    m_value.reset(new value_container<undefined_type>({}));
  }


  /************** 状態・属性 ***************/
  /** 値の型を取得 */
  const value_type getValueType() const { return m_value->value_type(); }

  /* undefined, null 確認用関数 */
  bool isUndefined() const  { return (m_value->value_type() == value_type::undefined); }
  bool isNull() const       { return (m_value->value_type() == value_type::null); }
  bool isNullOrUndefined() const { return isUndefined() || isNull(); }


  /************** プロパティ検索 ***************/
  /** object型に対する値の検索（const） */
  const json* find(const std::string& path, const char separator = '.') const {
    if(getValueType() != value_type::object) return nullptr;
    const auto pos = path.find(separator);
    auto&& obj = getValue<object_type>();
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
    if(getValueType() != value_type::object) return nullptr;
    const auto pos = path.find(separator);
    auto&& obj = getValue<object_type>();
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
};

#endif /* !defined(__h_json__) */
