#include "json.h"
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


  {
    auto x = json::array_type();
    x.push_back(json::create(12));
    auto y = x;
  }
  return 0;
}