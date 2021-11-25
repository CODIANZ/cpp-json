#include "cppjson/cppjson.h"
#include <iostream>
#include <map>
#include <list>

using namespace cppjson;

template <typename T> void printValue(const json& j) {
  try{
    std::cout << j.get<T>() << std::endl;
  }
  catch(std::exception* e){
    std::cerr << e->what() << std::endl;
  }
}

template <typename T> void printValue(const json* j) {
  if(j == nullptr) {
    std::cerr << "json* == nullptr" << std::endl;
  }
  else{
    printValue<T>(*j);
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
  printValue<int>(j);
  printValue<double>(j);
  
  j.set(12);
  j.set(12.5);
  j.set(12.5f);
  printValue<int>(j);
  printValue<double>(j);
  printValue<std::string>(j);

  j.set("abc");
  printValue<std::string>(j);
  printValue<int>(j); /* error */

  j.set(true);
  printValue<bool>(j);
  printValue<int>(j); /* error */
}

void test_006() {
  {
    json x({1, 2, 3});
    printValue<int>(x.get<json::array_type>()[2]);
  }
  {
    json x({ "a", "b", "c"});
    printValue<std::string>(x.get<json::array_type>()[1]);
  }

  {
    json x({
      {"abc", 1},
      {"aaa", "aaa"}
    });
    printValue<int>(path_util::find(x, "abc"));
    printValue<std::string>(path_util::find(x, "aaa"));
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
    printValue<double>(path_util::find(x, "ccc")->get<json::array_type>()[1]);
    printValue<int>(path_util::find(x, "ddd")->get<json::array_type>()[1]);
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
      }
    }
  )");
  auto jj = deserializer(ss).execute();
  printValue<int>(path_util::find(jj, "user_id"));

  printValue<int>(path_util::find(jj, "obj.value1"));

  auto value3 = path_util::find(jj, "obj.value3");  /** u03A9 -> 0xCE, 0xA9 */
  auto value3_arr = value3->get<json::array_type>();
  printValue<int>(value3_arr[0]);
  printValue<std::string>(value3_arr[2]);  
}

void test_008() {
  json j;
  j["abc"]["def"] = 123456;
  printValue<int>(path_util::find(j, "abc.def"));

  bool b = j[std::string("ddd")].is_undefined();
  std::cout << b << std::endl;
}


void test_009() {
  json j;
  j[1][0] = 555;
  printValue<int>(j[1][0]);
  printValue<nullptr_t>(j[0]);

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
  printValue<std::string>(path_util::find(j, "a"));
  printValue<std::string>(path_util::find(j, "b.b1"));
  printValue<std::string>(path_util::find(j, "b.b2"));
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
  printValue<std::string>(path_util::find(j, "a"));
  printValue<std::string>(path_util::find(j, "b.b1"));
  printValue<std::string>(path_util::find(j, "b.b2"));

  printValue<std::string>(path_util::find(jj, "a"));
  printValue<std::string>(path_util::find(jj, "b.b1"));
  printValue<std::string>(path_util::find(jj, "b.b2")); /** nullptr */
}

void test_012() {
  const std::string s = "123";
  auto j = json(s);       /** コピーコンストラクタ */
  printValue<std::string>(j);
  auto jj = json(std::string("abc")); /** 右辺値コンストラクタ */
  printValue<std::string>(jj);
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


  return 0;
}