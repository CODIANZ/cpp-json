#if !defined(__h_json_deserializer__)
#define __h_json_deserializer__

#include "json.h"
#include <istream>

class json_deserializer {
private:
  std::istream& m_stream;
  int m_line;
  int m_col;
  json m_root;

  bool getchar(char& c) {
    if(!m_stream.get(c)) return false;
    if(c == '\n'){
      m_line++;
      m_col = 1;
    }
    else {
      m_col++;
    }
    return true;
  }

  static bool is_blacket(char c) {
    return c == '\"';
  }

  static bool is_space(char c){
    return std::isspace(c);
  }

  static bool is_number_parts(char c){
    switch(c){
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case '-':
      case '+':
      case 'e':
      case 'E':
      case '.':
      {
        return true;
      }
    }
    return false;
  }

  void deserialize_object(json& j);
  void deserialize_array(json& j);
  void deserialize_string(json& j);
  void deserialize_number(json& j, const char first_char);
  
  void deserialize(json& j);
  void check_value(const std::string& s);


public:
  json_deserializer(std::istream& stream);
  ~json_deserializer() = default;
  const json getJson() const { return m_root; }
};

#endif /* !defined(__h_json_deserializer__) */
