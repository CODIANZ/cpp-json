# cppjson

## cppjson とは

json を C++ で取り扱うためのライブラリです。


## 方針

* C++ で取り回しが楽なように考慮する。（特に数値関連の相互変換）
* json は値を保持するオブジェクトとして定義し、その他の機能（シリアライズ、デシリアライズ、パスによるobjectの検索やアクセス）は分離する。
* header のみで構成する。
* もういい加減 C++14 使って良いかね。
* boostを使いたい場面もあるが堪えましょう。


## インストール

* このプロジェクトのパスをインクルードディレクトリパスに設定する。
* `#include <cppjson/cppjson.h>` と書けば全ての機能が使えます。


## 使い方

### 構築

#### コードでの初期化

```cpp
cppjson::json x = {
  {"ddd", 1},
  {"ccc", 1.5},
  {"ddd", {"abc", 1.5, 3, true}},
  {"eee", {
    {"1", 1.0},
    {"2", "2.0"}
  }}
};
```

#### デシリアライザを使った初期化

```cpp
std::stringstream ss(R"(
  {
    "user_id": 123, 
    "name": "hogehoge",
    "obj": {
      "value1": 1,
      "value2": "2",
      "value3": [
        1, true, "ABC"
      ]
    }
  }
)");
auto de = cppjson::deserializer(ss);
auto jj = de.value();
```

### 代入

```cpp
cppjson::json x;
x = 1;
x = "abc";
x = true;
x = nullptr;

/* array */
x = {"a", 1.5, true};

/* object */
x = {
  {"value1", 1},
  {"value2", "abc"},
  {"value3", {
    1, 2, false
  }}
};

/* copy */
auto y = x;

/* move */
auto z = std::move(x);
```

### 値の取得

```cpp
cppjson::json x = 123;
x.get<int>();
x.get<std::string>(); /* bad_cast */

x = {1, "abc", true, nullptr};
x.get<cppjson::json::array_type>()[0].get<int>();
x.get<cppjson::json::array_type>()[1].get<std::string>();
x.get<cppjson::json::array_type>()[2].get<bool>();
x.get<cppjson::json::array_type>()[3].is_null(); /* -> true */

x.get<cppjson::json::array_type>()[0].get<std::string>(); /* bad_cast */
```


## jsonの内部構造

### json の内部で保持する型

* std::string
* bool
* int64_t
* double
* undefined
* std::nullptr_t
* std::vector<cppjson::json>
* std::unordered<std::string, cppjson::json>

これらの型が、型消去した状態で `std::unique_ptr<>` にて保持されている。

なお、符号付整数、符号無整数は区別せず `int64_t` を採用しています。理由は符号である1ビットについて、数値範囲の云々言うのであれば、もはや多倍長演算が必要になるということじゃないかと思う次第。

### 数値の取り扱い

数値は内部で `int64_t` と `double` で保持していますが、値の設定、取得に関しては相互の変換を許容します。

```cpp
cppjson::json x = 123;  /* int64_t に変換されて保持 */
x.get<int>();     /* ok */
x.get<double>();  /* ok */
x = 1.23;
x.get<int>();     /* ok */
x.get<float>();   /* ok */
```

ただし、内部では `static_cast<>` で変換され、場合によっては、丸め込みが発生するので注意が必要です。


## ライセンス

このライブラリは [MITライセンス](http://opensource.org/licenses/MIT) です。　

Copyright (C) CODIANZ Inc.
