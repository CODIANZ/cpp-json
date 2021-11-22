#include "json.h"
#include "json_deserializer.h"
#include <iostream>


template <typename T> void printValue(const json& j) {
  try{
    std::cout << j.getValue<T>() << std::endl;
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
void test_0() {
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
void test_01() {
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

/** setValue */
void test_02() {
  json a;
  a.setValue(std::string("abc"));
  a.setValue(true);
  a.setValue(nullptr);
  a.setValue({1, "abc", true});
  a.setValue({{"key1", "value1"}, {"key2", 1.23}});
  a.setValue(123456789ll);
  a.setValue(1.2345678);
  a.setValue(456);
  a.setValue(1.2345678f);
  a.setValue("c-string");
}

/** json to json */
void test_03() {
  json a, b;
  a = "abc";
  b = a;
  a = std::move(b);
}




void test_1() {
  json j;
  j.setValue("aaa");

  j.setValue(12LL);
  printValue<int>(j);
  printValue<double>(j);
  
  j.setValue(12);
  j.setValue(12.5);
  j.setValue(12.5f);
  printValue<int>(j);
  printValue<double>(j);
  printValue<std::string>(j);

  j.setValue("abc");
  printValue<std::string>(j);
  printValue<int>(j);

  j.setValue(true);
  printValue<bool>(j);
  printValue<int>(j);
}

void test_2() {
  std::stringstream ss(R"(
    {
      "user_id": 123, 
      "name": "Alice",
      "obj": {
        "value1": 1,
        "value2": "2",
        "value3": [
          1, true, "ABC"
        ]
      }
    }
  )");
  auto de = json_deserializer(ss);
  auto jj = de.getJson();
  printValue<int>(jj.find("user_id"));

  printValue<int>(jj.find("obj.value1"));

  auto value3 = jj.find("obj.value3");
  auto value3_arr = value3->getValue<json::array_type>();
  printValue<int>(value3_arr[0]);
  printValue<std::string>(value3_arr[2]);  
}

void test_3() {
  {
    json x({1, 2, 3});
    printValue<int>(x.getValue<json::array_type>()[2]);
  }
  {
    json x({ "a", "b", "c"});
    printValue<std::string>(x.getValue<json::array_type>()[1]);
  }

  {
    json x({
      {"abc", 1},
      {"aaa", "aaa"}
    });
    printValue<int>(x.find("abc"));
    printValue<std::string>(x.find("aaa"));
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
    printValue<double>(x.find("ccc")->getValue<json::array_type>()[1]);
    printValue<int>(x.find("ddd")->getValue<json::array_type>()[1]);
  }
  {
    json y = {1, "abc", true};
    json x;
    x = 1;
    x.setValue({{"aa", 2.54}, {"bbb", true}});
    x.setValue({1, 2, "true", true});
    x = {1, "abc"};
    x = {{"aa", 1}, {"bb", 22}};
  }
}

int main(void) {

  std::cout << "********** test_1() **********" << std::endl;
  test_1();

  std::cout << "********** test_2() **********" << std::endl;
  test_2();

  std::cout << "********** test_3() **********" << std::endl;
  test_3();

  return 0;
}