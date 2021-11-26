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
      }
    }
    else{
      if(v == exceptedValue){
        std::cerr << "ng: " << v << " (same value)" << std::endl;
      }
      else{
        std::cerr << "ok: " << v << " (excepted: " << exceptedValue << ")" << std::endl;
      }
    }
  }
  catch(std::exception* e){
    if(exceptedCompareResult == compare::same){
      std::cerr << "ng: " << e->what() << std::endl;
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
  json a;
  json b(std::string("abc"));
  json c(true);
  json d(nullptr);
  json e({1, "abc", true});
  json f({{"key1", "value1"}, {"key2", 1.23}});
  json g(123456789ll);
  json h(1.2345678);
  json i(456);
  json j(1.2345678f);
  json k("c-string");
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

/** json = json */
void test_004() {
  json a, b;
  a = "abc";
  b = a;
  a = std::move(b);
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
}


void test_009() {
  json j;
  j[1][0] = 555;
  valueValidation<int>(j[1][0], 555, compare::same);
  valueValidation<int>(j[0], 0, compare::different);

  const auto& jj = j;

  const auto bb = jj[50];
  std::cout << bb.is_null_or_undefined() << std::endl;

  const bool b = j[50].is_undefined();
  std::cout << b << std::endl;
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

  return 0;
}