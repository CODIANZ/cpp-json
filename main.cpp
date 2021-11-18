#include "json.h"
#include "json_deserializer.h"
#include <iostream>


template <typename T> void printValue(const json& j) {
  try{
    std::cout << j.getValue<T>() << std::endl;
  }
  catch(std::exception* e){
    std::cout << e->what() << std::endl;
  }
}

int main(void) {
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
  auto uid = jj.searchValue("user_id");
  printValue<int>(uid);

  auto v1 = jj.searchValue("obj.value1");
  printValue<int>(v1);

  auto v3 = jj.searchValue("obj.value3");
  auto v3a = v3.getValue<json::array_type>();
  printValue<int>(v3a[0]);
  printValue<std::string>(v3a[2]);

  {
    auto x = json::array_type();
    x.push_back(json::create(12));
    auto y = x;
  }
  return 0;
}