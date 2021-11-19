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
      {"ccc", {"abc", "def"}},
      {"ddd", {1, 2}},
      {"eee", {
        {"1", 1.0},
        {"2", "2.0"}
      }}
    };
    printValue<std::string>(x.find("ccc")->getValue<json::array_type>()[1]);
    printValue<int>(x.find("ddd")->getValue<json::array_type>()[1]);
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