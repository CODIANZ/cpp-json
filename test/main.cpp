#include <cppjson/cppjson.h>
#include <iostream>
#include <map>
#include <list>

using namespace cppjson;

enum class compare { same, different };

template<typename JSON_VALUE_TYPE, typename EXCEPTED_VALUE_TYPE>
void valueValidation(const json& j, const EXCEPTED_VALUE_TYPE& exceptedValue, const compare exceptedCompareResult) {
  try{
    const auto& v = j.get<JSON_VALUE_TYPE>();
    if(exceptedCompareResult == compare::same){
      if(v == exceptedValue){
        std::cout << "ok: " << v << std::endl;
      }
      else{
        std::cerr << "ng: " << v << " (excepted: " << exceptedValue << ")" << std::endl;
        assert(false);
      }
    }
    else{
      if(v == exceptedValue){
        std::cerr << "ng: " << v << " (same value)" << std::endl;
        assert(false);
      }
      else{
        std::cout << "ok: " << v << " (excepted: " << exceptedValue << ")" << std::endl;
      }
    }
  }
  catch(std::exception* e){
    if(exceptedCompareResult == compare::same){
      std::cerr << "ng: " << e->what() << std::endl;
      assert(false);
    }
    else{
      std::cout << "ok: " << e->what() << std::endl;
    }
  }
}

template <typename JSON_VALUE_TYPE, typename EXCEPTED_VALUE_TYPE>
void valueValidation(const json* j, const EXCEPTED_VALUE_TYPE& exceptedValue, const compare exceptedCompareResult) {
  if(j == nullptr) {
    if(exceptedCompareResult == compare::same){
      std::cerr << "ng: " << "json* == nullptr (excepted: " << exceptedValue << ")" << std::endl;
      assert(false);
    }
    else{
      std::cout << "ok: " << "json* == nullptr (excepted: " << exceptedValue << ")" << std::endl;
    }
  }
  else{
    valueValidation<JSON_VALUE_TYPE>(*j, exceptedValue, exceptedCompareResult);
  }
}

/** constructor */
void test_001() {
  { json a; }
  { json a(std::string("abc")); }
  { json a(true); }
  { json a(nullptr); }
  { json a({1, "abc", true}); }
  { json a({{"key1", "value1"}, {"key2", 1.23}}); }
  { json a(123456789ll); }
  { json a(1.2345678); }
  { json a(456); }
  { json a(1.2345678f); }
  { json a("c-string"); }
}

/** operator = */
void test_002() {
  json a;
  a = std::string("abc");
  a = true;
  a = nullptr;
  a = {1, "abc", true};
  a = {{"key1", "value1"}, {"key2", 1.23}};
  a = 123456789ll;
  a = 1.2345678;
  a = 456;
  a = 1.2345678f;
  a = "c-string";
}

/** setter */
void test_003() {
  json a;
  a.set(std::string("abc"));
  a.set(true);
  a.set(nullptr);
  a.set({1, "abc", true});
  a.set({{"key1", "value1"}, {"key2", 1.23}});
  a.set(123456789ll);
  a.set(1.2345678);
  a.set(456);
  a.set(1.2345678f);
  a.set("c-string");
}

/** copy & move */
void test_004() {
  json a, b;
  a = "abc";
  b = a; /* copy */
  assert(a.get<std::string>().c_str() != b.get<std::string>().c_str());

  valueValidation<std::string>(a, "abc", compare::same);
  valueValidation<std::string>(b, "abc", compare::same);
  a = std::move(b);
  valueValidation<std::string>(a, "abc", compare::same);
  valueValidation<std::string>(b, "abc", compare::different); /** b = undefined */

  auto s = a.release<std::string>();
  assert(s == "abc");
  valueValidation<std::string>(a, "abc", compare::different); /** a = undefined */

  a = {
    { "1", {
      {"a", "abc"},
      {"b", "bcd"}
    }},
    { "2", "234" }
  };

  const char* ptr = a["1"]["a"].get<std::string>().c_str();

  b = std::move(a["1"]);
  valueValidation<int>(a["1"], 0, compare::different); /** = undefined */
  valueValidation<std::string>(b["a"], "abc", compare::same);
  json::object_type m = b.release<json::object_type>();
  valueValidation<std::string>(b["a"], "abc", compare::different); /** = undefined */
  
  const char* ptr2 = m["a"].get<std::string>().c_str();
  assert(ptr == ptr2);
}

/** get */
void test_005() {
  json j;
  j.set("aaa");

  j.set(12LL);
  valueValidation<int64_t>(j, 12LL, compare::same);
  valueValidation<int>(j, 12, compare::same);
  valueValidation<double>(j, 12.0, compare::same);
  
  j.set(12);
  j.set(12.5);
  j.set(12.5f);
  valueValidation<int>(j, 12, compare::same);
  valueValidation<float>(j, 12.5, compare::same);
  valueValidation<std::string>(j, "12.5", compare::different);

  j.set("abc");
  valueValidation<std::string>(j, "abc", compare::same);
  valueValidation<uint16_t>(j, 0xabc, compare::different);

  j.set(true);
  valueValidation<bool>(j, true, compare::same);
  valueValidation<int>(j, 1, compare::different);
}

void test_006() {
  {
    json x({1, 2, 3});
    valueValidation<int>(x.get<json::array_type>()[2], 3, compare::same);
  }
  {
    json x({ "a", "b", "c"});
    valueValidation<std::string>(x.get<json::array_type>()[1], "b", compare::same);
  }

  {
    json x({
      {"abc", 1},
      {"aaa", "aaa"}
    });
    valueValidation<int>(path_util::find(x, "abc"), 1, compare::same);
    valueValidation<std::string>(path_util::find(x, "aaa"), "aaa", compare::same);
  }
  {
    json x = {
      {"aaa", 1},
      {"bbb", 1.5},
      {"ccc", {"abc", 1.234, true}},
      {"ddd", {1, 2}},
      {"eee", {
        {"1", 1.0},
        {"2", "2.0"}
      }}
    };
    valueValidation<double>(path_util::find(x, "ccc")->get<json::array_type>()[1], 1.234, compare::same);
    valueValidation<int>(path_util::find(x, "ddd")->get<json::array_type>()[1], 2, compare::same);
  }
  {
    json y = {1, "abc", true};
    json x;
    x = 1;
    x.set({{"aa", 2.54}, {"bbb", true}});
    x.set({1, 2, "true", true});
    x = {1, "abc"};
    x = {{"aa", 1}, {"bb", 22}};
  }
}

void test_007() {
  std::stringstream ss(R"(
    {
      "user_id": 123,   /* abc
      ****/
      "name": "Alice", // name
      "obj": {
        "value1": 1,
        "value2": "2",
        "value3": [
          1, true, /* comment */ "ABC\n\u03A9DEF"
        ]
      },
      "\r\nkey" : null
    }
  )");
  auto jj = deserializer(ss).execute();
  valueValidation<int>(path_util::find(jj, "user_id"), 123, compare::same);

  valueValidation<int>(path_util::find(jj, "obj.value1"), 1, compare::same);

  auto value3 = path_util::find(jj, "obj.value3");  /** u03A9 -> 0xCE, 0xA9 */
  auto value3_arr = value3->get<json::array_type>();
  valueValidation<int>(value3_arr[0], 1, compare::same);
  valueValidation<std::string>(value3_arr[2], "ABC\nΩDEF", compare::same);  
}

void test_008() {
  json j;
  j["abc"]["def"] = 123456;
  valueValidation<int>(path_util::find(j, "abc.def"), 123456, compare::same);

  bool b = j[std::string("ddd")].is_undefined();
  std::cout << b << std::endl;
  assert(b == true);
}


void test_009() {
  json j;
  j[1][0] = 555;
  valueValidation<int>(j[1][0], 555, compare::same);
  valueValidation<int>(j[0], 0, compare::different);

  const auto& jj = j;

  const auto bb = jj[50];
  std::cout << bb.is_null_or_undefined() << std::endl;
  assert(bb.is_null_or_undefined() == true);

  const bool b = j[50].is_undefined();
  std::cout << b << std::endl;
  assert(b == false); /* j[50] == nullptr */
}

void test_010() {
  json j = {
    {"a",  "_a"},
    {"b", {
      {"b1", "_b1"}
    }}
  };
  path_util::put(j, "b.b2", "_b2");
  valueValidation<std::string>(path_util::find(j, "a"), "_a", compare::same);
  valueValidation<std::string>(path_util::find(j, "b.b1"), "_b1", compare::same);
  valueValidation<std::string>(path_util::find(j, "b.b2"), "_b2", compare::same);
}


void test_011() {
  json j = {
    {"a",  "_a"},
    {"b", {
      {"b1", "_b1"}
    }}
  };
  json jj = j.clone();

  path_util::put(j, "b.b2", "_b2");
  valueValidation<std::string>(path_util::find(j, "a"), "_a", compare::same);
  valueValidation<std::string>(path_util::find(j, "b.b1"), "_b1", compare::same);
  valueValidation<std::string>(path_util::find(j, "b.b2"), "_b2", compare::same);

  valueValidation<std::string>(path_util::find(jj, "a"), "_a", compare::same);
  valueValidation<std::string>(path_util::find(jj, "b.b1"), "_b1", compare::same);
  valueValidation<std::string>(path_util::find(jj, "b.b2"), "_b2", compare::different);
}

void test_012() {
  const std::string s = "123";
  auto j = json(s);       /** コピーコンストラクタ */
  valueValidation<std::string>(j, "123", compare::same);
  auto jj = json(std::string("abc")); /** 右辺値コンストラクタ */
  valueValidation<std::string>(jj, "abc", compare::same);
}

void test_013() {
  json x = {
    {"aaa", 1},
    {"bbb", 1.5},
    {"ccc", {"abc", 1.234, true}},
    {"ddd", {1, 2}},
    {"eee", {
      {"1", 1.0},
      {"2", "2.0"}
    }}
  };
  std::cout << serializer(x).execute() << std::endl;
  std::cout << serializer(x, " ").execute() << std::endl;
}

void test_014() {
  std::vector<int> iv({1, 2, 3, 4});
  json j = array_util::to_json(iv.begin(), iv.end());
  std::cout << serializer(j).execute() << std::endl;

  std::list<std::string> sv({"abc", "def", "ghi"});
  j = array_util::to_json(sv);
  std::cout << serializer(j).execute() << std::endl;

  j = array_util::create([](auto& arr){
    arr.push_back(true);
    arr.push_back(123);
    arr.push_back("baz");
  });
  std::cout << serializer(j).execute() << std::endl;

  array_util::edit(j, [](auto& arr){
    arr.pop_back();
    arr.push_back({
      {"inner", 1}
    });
  });
  std::cout << serializer(j).execute() << std::endl;
}

void test_015() {
  std::map<std::string,int> im({
    {"abc", 1},
    {"def", 2}
  });
  json j = object_util::to_json(im.begin(), im.end());
  std::cout << serializer(j).execute() << std::endl;

  std::map<std::string, bool> bm({
    {"abc", true},
    {"def", false}
  });
  j = object_util::to_json(bm);
  std::cout << serializer(j).execute() << std::endl;

  j = object_util::create([](auto& obj){
    obj["foo"] = true;
    obj["bar"] = 123;
    obj["baz"] = "baz";
  });
  std::cout << serializer(j).execute() << std::endl;

  object_util::edit(j, [](auto& obj){
    obj.erase("baz");
    obj.insert({"inner", {1, 2.4, true}});
  });
  std::cout << serializer(j).execute() << std::endl;
}

void test_016() {
  std::map<std::string,int> im({
    {"abc", 1},
    {"def", 2}
  });
  json::sp j = object_util::to_json(im.begin(), im.end()).to_shared();
  valueValidation<int>((*j)["abc"], 1, compare::same);
}

void test_017() {
  json j(10);
  assert(j.acquirable<int>() == true);
  assert(j.acquirable<bool>() == false);
  assert(j.acquirable<std::string>() == false);
  j = "abc";
  assert(j.acquirable<int>() == false);
  assert(j.acquirable<bool>() == false);
  assert(j.acquirable<std::string>() == true);
}

void test_018() {
  /**
   * array内に array や object がネストする場合、
   * array の初期化は、initalizer_list　ではなく可変引数テンプレートが採用されるため、
   * 入れ子になった array や object は json を明示する必要がある。
   * 改善しようか悩んだが、下記のようなケースを考えると、これが object なのか、文字列の２次元配列なのか
   * 判別しようがなく、おそらくコンパイラも ambiguous だと怒られるのオチなので、
   * この仕様のままとする。
   * {
   *   {"key1", "value1"},
   *   {"key2", "value2"}
   * }
   **/
  json j = {
    1,
    json{ 2, 3 }
  };
  valueValidation<int>(j[0], 1, compare::same);
  valueValidation<int>(j[1][0], 2, compare::same);
  valueValidation<int>(j[1][1], 3, compare::same);
  
  j = {
    json{ 1, true },
    3,
    json{
      {"key1", "value1"},
      {"key2", "value2"}
    }
  };
  valueValidation<int>(j[0][0], 1, compare::same);
  valueValidation<bool>(j[0][1], true, compare::same);
  valueValidation<int>(j[1], 3, compare::same);
  valueValidation<std::string>(j[2]["key1"], "value1", compare::same);
  valueValidation<std::string>(j[2]["key2"], "value2", compare::same);

  /** object内の array や object のネストは問題ない */
  j = {
    {"key1", "value1"},
    {"key2", {"value2-1", 123}},
    {"key3", {
      {"value3-1", 1},
      {"value3-2", false}
    }}
  };
  valueValidation<std::string>( j["key1"], "value1", compare::same);
  valueValidation<std::string>( j["key2"][0], "value2-1", compare::same);
  valueValidation<int>(         j["key2"][1], 123, compare::same);
  valueValidation<int>(         j["key3"]["value3-1"], 1, compare::same);
  valueValidation<bool>(        j["key3"]["value3-2"], false, compare::same);
}

int main(void) {

  std::cout << "********** test_001() **********" << std::endl;
  test_001();

  std::cout << "********** test_002() **********" << std::endl;
  test_002();

  std::cout << "********** test_003() **********" << std::endl;
  test_003();

  std::cout << "********** test_004() **********" << std::endl;
  test_004();

  std::cout << "********** test_005() **********" << std::endl;
  test_005();

  std::cout << "********** test_006() **********" << std::endl;
  test_006();

  std::cout << "********** test_007() **********" << std::endl;
  test_007();

  std::cout << "********** test_008() **********" << std::endl;
  test_008();

  std::cout << "********** test_009() **********" << std::endl;
  test_009();

  std::cout << "********** test_010() **********" << std::endl;
  test_010();

  std::cout << "********** test_011() **********" << std::endl;
  test_011();

  std::cout << "********** test_012() **********" << std::endl;
  test_012();

  std::cout << "********** test_013() **********" << std::endl;
  test_013();

  std::cout << "********** test_014() **********" << std::endl;
  test_014();

  std::cout << "********** test_015() **********" << std::endl;
  test_015();

  std::cout << "********** test_016() **********" << std::endl;
  test_016();

  std::cout << "********** test_017() **********" << std::endl;
  test_017();

  std::cout << "********** test_018() **********" << std::endl;
  test_018();

  return 0;
}