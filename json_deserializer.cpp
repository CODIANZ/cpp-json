#include "json_deserializer.h"

json_deserializer::json_deserializer(std::istream& stream) :
  m_stream(stream),
  m_line(1),
  m_col(1)
{
  deserialize(m_root);
}

void json_deserializer::deserialize_object(json& j)
{
  auto obj = json::object_type();

  enum class mode {
    find_key_or_close,
    proceed_key,
    find_separator,
    find_comma_or_close
  };
  mode m = mode::find_key_or_close;

  std::stringstream key;
  char c;
  while(getchar(c)){
    switch(m){
      case mode::find_key_or_close: {
        if(c == '}'){
          /* end of object */
          j.setValue(obj);
          return;
        }  
        else if(is_blacket(c)) {
          m = mode::proceed_key;
        }
        else if(is_space(c)) {
        }
        else {
          /** throw error */
        }
        break;
      }
      case mode::proceed_key: {
        if(is_blacket(c)){
          m = mode::find_separator;
        }
        else{
          key << c;
        }
        break;
      }
      case mode::find_separator: {
        if(c == ':'){
          json inner;
          deserialize(inner);
          obj.insert({key.str(), inner});
          key.str("");
          key.clear();
          m = mode::find_comma_or_close;
        }
        else if(is_space(c)){
        }
        else {
          /** throw error */
        }
        break;
      }
      case mode::find_comma_or_close: {
        if(c == '}'){
          /* end of object */
          j.setValue(obj);
          return;
        }  
        else if(c == ',') {
          m = mode::find_key_or_close;
        }
        else if(is_space(c)) {
        }
        else {
          /** throw error */
        }
        break;
      }
    }
  }
  /** throw error */
}

void json_deserializer::deserialize_array(json& j)
{
  auto arr = json::array_type();

  char c;
  /* unget() をするため、getchar() は使わない */
  while(m_stream.get(c)){
    m_col++;
    if(c == ']'){
      /* end of array */
      j.setValue(arr);
      return;
    }
    else if(c == ','){
      if(arr.size() == 0){
        /** throw error */
      }
      else{
      }
    }
    else if(is_space(c)){
    }
    else{
      m_col--;
      m_stream.unget();
      json inner;
      deserialize(inner);
      arr.push_back(inner);
    }
  }
  /** throw error */
}

void json_deserializer::deserialize_string(json& j)
{
  std::stringstream ss;
  char c;
  while(getchar(c)){
    if(is_blacket(c)){
      j.setValue(ss.str());
      return;
    }
    else{
      ss << c;
    }
  }
  /** throw error */
}

void json_deserializer::deserialize_number(json& j, const char first_char)
{
  char c;
  std::stringstream ss;
  ss << first_char;
  bool bFloat = false;
  /* unget() をするため、getchar() は使わない */
  while(m_stream.get(c)){
    if(!is_number_parts(c)){
      m_stream.unget(); /* １文字戻す */
      break;
    }
    else{
      m_col++;  /* Number型なので改行の考慮は不要 */
      if(c == '.' || c == 'e' || c == 'E'){
        bFloat = true;
      }
      ss << c;
    }
  }
  auto&& s = ss.str();
  std::vector<char> verr(s.size());
  char* err = verr.data();
  if(bFloat){
    auto v = ::strtod(ss.str().c_str(), &err);
    if(verr[0] != 0){
      /** throw error 数値に変換できない */
    }
    j.setValue(v);
  }
  else{
    auto v = ::strtoll(ss.str().c_str(), &err, 10);
    if(verr[0] != 0){
      /** throw error 数値に変換できない */
    }
    j.setValue(v);
  }
}

void json_deserializer::check_value(const std::string& s)
{
  for(auto a : s){
    char c;
    if(!getchar(c)){
      /** throw error */
    }
    else if(c != a){
      /** throw error */
    }
  }
} 

void json_deserializer::deserialize(json& j)
{
  char  c;
  while(getchar(c)){
    if(c == '{'){
      deserialize_object(j);
      return;
    }
    else if(c == '['){
      deserialize_array(j);
      return;
    }
    else if(c == 't'){
      check_value("rue");
      j.setValue(true);
      return;
    }
    else if(c == 'f'){
      check_value("alse");
      j.setValue(false);
      return;
    }
    else if(c == 'n'){
      check_value("ull");
      j.setValue(nullptr);
      return;
    }
    else if(is_blacket(c)){
      deserialize_string(j);
      return;
    }
    else if(is_number_parts(c)){
      deserialize_number(j, c);
      return;
    }
    else if(is_space(c)){
    }
    else{
      /** throw error */
    }
  }
  /** throw error */
}
