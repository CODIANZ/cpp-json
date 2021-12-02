
#if !defined(__cppjson_h_array__)
#define __cppjson_h_array__

#include "json.h"
#include "object.h"

namespace cppjson {

/**
 * array は復数の型が混在するコンテナなので C++ で表現するのが難しい。
 * そこで array という json から派生したクラスを設けて、
 * 可変引数テンプレートで初期化を行うという手法を採用した。
 * しかし、arrayの要素に array や object がネストする場合、
 * initalizer_list だと入れ子になった　initializer_list を判別できない。
 * そこで、入れ子になった array や object は明示する必要がある。
 * auto j = array{
 *   array{ 1, true },
 *   3,
 *   object{ // json でも可
 *     {"key1", "value1"},
 *     {"key2", "value2"}
 *   }
 * };
 * なお、object は json とほぼ等価であるため、json を使用しても良い。
 **/

/** 可変引数テンプレートで array の初期化を行うための json のサブクラス */
class array : public json {
private:
  /** json の配列による設定の継続処理 */
  template <typename T, typename ...ARGS> static void pushValues(array_type& arr, T&& v, ARGS ...args){
    pushTypedValue(arr, std::forward<T>(v));
    pushValues(arr, std::forward<ARGS>(args)...);
  }

  /** json の配列による設定のゴール地点 */
  static void pushValues(array_type& arr) {};

  /** 許容される型のみ arr に追加（追加できない型はコンパイルエラー） */
  template <
    typename T,
    std::enable_if_t<
      json::is_number_type<T>::value ||
      value_type_traits<T>::available ||
      std::is_same<T, const char*>::value ||
      std::is_same<T, json>::value ||
      std::is_same<T, array>::value ||
      std::is_same<T, object>::value
      , bool
    > = true
  > static void pushTypedValue(array_type& arr, T&& v) {
    arr.push_back(std::forward<T>(v));
  }

public:
  template <typename ...ARGS> array(ARGS ...args) {
    auto arr = array_type();
    pushValues(arr, std::forward<ARGS>(args)...);
    set(std::move(arr));
  }

  struct util {
    template<typename ITER> static json to_json(ITER begin, ITER end) {
      return create([&](json::array_type& arr){
        arr.insert(arr.begin(), begin, end);
      });
    }

    template<typename T> static json to_json(const T& src){
      return to_json(std::cbegin(src), std::cend(src));
    }

    static json create(std::function<void(json::array_type&)> f) {
      json j = create();
      f(j.get<json::array_type>());
      return j;
    }

    static json create() {
      return json::array_type();
    }

    static json& edit(json& j, std::function<void(json::array_type&)> f){
      auto& arr = j.get<json::array_type>();
      f(arr);
      return j;
    }
  };
};

} /** namespace cppjson */

#endif /** !defined(__cppjson_h_array__) */