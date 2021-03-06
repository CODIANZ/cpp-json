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

* このプロジェクトのビルドは不要です。
* このプロジェクト内の `include` ディレクトリをインクルード検索ディレクトリパスに設定します。
* `#include <cppjson/cppjson.h>` と書けば全ての機能が使えます。


## 使い方

### コードでの初期化

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

### シリアライズとデシリアラズ

```cpp
std::stringstream ss(R"(
  {
    "user_id": 123, 
    "name": "hogehoge" /* block comment */,
    "obj": { // line comment
      "value1": 1,
      "value2": "2",
      "value3": [
        1, true, "ABC"
      ]
    }
  }
)");
/** シリアライズ */
json j = cppjson::deserializer(ss).execute();
/** デシリアライズ */
std::string se = cppjson::serializer(j).execute();
```

なお、JSON には準拠していませんが、ラインコメントとブロックコメントが便利すぎるので対応しています。


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

数値（整数・浮動小数点）型の代入は内部で `int64_t` と `double` に `static_cast<>` して保持します。


### 値の取得

```cpp
cppjson::json x = 123;
x.get<int>();
x.get<std::string>(); /* bad_cast */

/** 配列のアクセス */
x = {1, "abc", true, nullptr};
x.get<cppjson::json::array_type>()[0].get<int>();
x.get<cppjson::json::array_type>()[1].get<std::string>();
x.get<cppjson::json::array_type>()[2].get<bool>();
x.get<cppjson::json::array_type>()[3].is_null(); /* -> true */
x.get<cppjson::json::array_type>()[0].get<std::string>(); /* bad_cast */

/** 省略も可能です */
x[0].get<int>();
x[1].get<std::string>();
x[2].get<bool>();
x[3].is_null();

/** オブジェクト型（連想配列）のアクセス */
x = {{"a", 1}, {"b", false}};
x.get<cppjson::json::object_type>()["a"].get<int>();
x.get<cppjson::json::object_type>()["b"].get<bool>();

/* 省略も可能です */
x["a"].get<int>();
x["b"].get<bool>();
```

基本的に参照を返却しますが、数値（整数・浮動小数点）型で `int64_t` と `double` に `static_cast<>` するため実体が返却されます。

## jsonの内部構造

### json の内部で保持する型

* int64_t (*1)
* double
* bool
* std::nullptr_t
* std::string (*2)
* std::vector\<cppjson::json\> (*2)
* std::unordered\<std::string, cppjson::json\> (*2)

これらの型を union で共有メモリ上にマッピングした状態で保持しています。

(*1) ... 符号付整数、符号無整数は区別せず `int64_t` を採用しています。理由は符号である1ビットについて、数値範囲の云々言うのであれば、もはや多倍長演算が必要になるということじゃないかと思う次第です。

(*2) ... ポインタにて保持。

### undefined の取り扱い

* `undefined` は未初期化を表現するものですが、この `undefined` という「モノ」代入することはできません。（boostで云うところの `boost::none` は存在しません） 
* `is_undefined()` が `true` の場合には値を取り出すことはできません。
* `undefined` が生成される（生成する）タイミングは下記の通りです。
  * デフォルトコンストラクタ `cppjson::json()` を呼び出した場合。
  * object型に対してキーを添字アクセスをした際に、該当のキーが存在しない場合の戻り値の中身。


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

## 注意事項

### array の初期化

json には object を生成することはできますが、array を生成することはできません。
これは、C++の言語仕様上、 `{{"a", "b"}}` が `a` というプロパティを持つオブジェクトなのか、 `a`と`b`を持つ配列を１個保有する配列（二次元配列）なのかが判別できないからです。
そこで、cppjsonは array を保持する json 生成を json　のサブクラスで実装しています。
なお、 object を保持する json 生成も明示できるようサブクラスが用意されています。

```cpp
/** object を生成 */
json obj = {{"key", "value"}};
json obj2 = object{{"key", "value"}};

/** array を生成 */
json arr = array{"1", "2"};
```

### array 内に array や object がネストする場合

array内に array や object がネストする場合、array の初期化は、initalizer_list　ではなく可変引数テンプレートが採用されるため、入れ子になった array や object はこれらを明示する必要があります。

```cpp
json arr = {
  array{ 1, true },
  3,
  object{
    {"key1", "value1"},
    {"key2", "value2"}
  }
};
```

object内の array や object のネストは問題ありません。

```cpp
json obj = {
  {"key1", "value1"},
  {"key2", {"value2-1", 123}},
  {"key3", {
    {"value3-1", 1},
    {"value3-2", false}
  }}
};
```

また、下記のように object なのか string の二次元配列なのか曖昧な場合は、objectが優先的に採用されます。

```cpp
json obj = {
  {"1", "2"},
  {"3", "4"}
};
```

したがって、string の二次元配列にしたい場合には下記のように明示をする。

```cpp
json arr = {
  array{"1", "2"},
  array{"3", "4"}
};
```


## ライセンス

このライブラリは [MITライセンス](http://opensource.org/licenses/MIT) です。　

Copyright (C) CODIANZ Inc.
