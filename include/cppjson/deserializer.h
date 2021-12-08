#if !defined(__cppjson_h_deserializer__)
#define __cppjson_h_deserializer__

#include "json.h"
#include <istream>
#include <array>

namespace cppjson {
class deserializer {
private:
  class stream {
  private:
    std::istream& m_is;
    int m_line;
    int m_col;
    std::array<int, 6> m_buff;

  public:
    stream(std::istream& stream)
      : m_line(0), m_col(0), m_is(stream)
    {
      for(auto i = 0; i < m_buff.size(); i++){
        char c;
        if(m_is.get(c)){
          m_buff[i] = static_cast<int>(c);
        }
        else{
          m_buff[i] = -1;
        }
      }
    }

    void next(std::size_t n) {
      if(m_buff[0] == -1) return;

      m_col += n;

      for(auto i = n; i < m_buff.size(); i++){
        m_buff[i - n] = m_buff[i];
      }

      for(auto i = m_buff.size() - n; i < m_buff.size(); i++)
      {
        char cc;
        if(m_is.get(cc)){
          m_buff[i] = static_cast<int>(cc);
        }
        else{
          m_buff[i] = -1;
        }
      }
    }

    void newLine() {
      m_line++;
      m_col = 0;
    }

    char operator [](std::size_t n) const{
      auto v = m_buff[n];
      if(v == -1) return '\0';
      return v == -1 ? '\0' : static_cast<char>(v);
    }

    bool eof() const {
      return m_buff[0] == -1;
    }

    std::string str(std::size_t n) const {
      std::string s;
      for(auto i = 0; i < n; i++){
        auto ch = m_buff[i];
        if(ch == -1) break;
        s += static_cast<char>(ch);
      }
      return s;
    }


    int line() const { return m_line + 1; }
    int col() const { return m_col + 1; }
  };

  stream m_stream;

  [[noreturn]] void throwError(const std::string& err) {
    std::stringstream ss;
    ss << "line(" << m_stream.line() << "), col(" << m_stream.col() << ") : " << err;
    throw bad_json(ss.str());
  }

  static bool is_blacket(char c) {
    return c == '\"';
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

  void unescape(std::ostream& os)
  {
    m_stream.next(1); /** \ をスキップ */
    if(m_stream.eof()){
      throwError("illegal eof");
    }
    switch(m_stream[0]){
      case '"':
      case '\\':
      case '/':
      {
        os << m_stream[0];
        m_stream.next(1);
        return;
      }
      case 'b':
      {
        os << '\b';
        m_stream.next(1);
        return;
      }
      case 'f':
      {
        os << '\f';
        m_stream.next(1);
        return;
      }
      case 'n':
      {
        os << '\n';
        m_stream.next(1);
        return;
      }
      case 'r':
      {
        os << '\r';
        m_stream.next(1);
        return;
      }
      case 't':
      {
        os << '\t';
        m_stream.next(1);
        return;
      }
      case 'u':
      {
        m_stream.next(1);
        break;  /** utf16 処理へ移行 */
      }
      default:
      {
        throwError("invalid escape character");
      }
    }
    
    /** 4つの HEX 文字からutf16文字コードに変換 */
    uint16_t unicode = 0;
    for(auto i = 0; i < 4; i++){
      unicode <<= 4;
      switch(m_stream[i]){
          case '0': { unicode |= 0x0; break; }
          case '1': { unicode |= 0x1; break; }
          case '2': { unicode |= 0x2; break; }
          case '3': { unicode |= 0x3; break; }
          case '4': { unicode |= 0x4; break; }
          case '5': { unicode |= 0x5; break; }
          case '6': { unicode |= 0x6; break; }
          case '7': { unicode |= 0x7; break; }
          case '8': { unicode |= 0x8; break; }
          case '9': { unicode |= 0x9; break; }
          case 'a':
          case 'A': { unicode |= 0xA; break; }
          case 'b':
          case 'B': { unicode |= 0xB; break; }
          case 'c':
          case 'C': { unicode |= 0xC; break; }
          case 'd':
          case 'D': { unicode |= 0xD; break; }
          case 'e':
          case 'E': { unicode |= 0xE; break; }
          case 'f':
          case 'F': { unicode |= 0xF; break; }
          default: {
            throwError("invalid unicode character");
          }
      }
    }
    
    m_stream.next(4);

    /** utf16 -> utf8 へ変換 */
    if(unicode <= 0x007F){
      os << static_cast<char>(unicode);
    }
    else if(unicode <= 0x07FF){
      os << static_cast<char>(0b11000000 + ((unicode >>  6) & 0b00011111))
         << static_cast<char>(0b10000000 + ( unicode        & 0b00111111));
    }
    else{
      os << static_cast<char>(0b11100000 + ((unicode >> 12) & 0b00001111))
         << static_cast<char>(0b10000000 + ((unicode >>  6) & 0b00111111))
         << static_cast<char>(0b10000000 + ( unicode        & 0b00111111));
    }
  }  

  void deserialize_object(json& j)
  {
    m_stream.next(1); /** { をスキップ */

    auto obj = json::object_type();

    enum class mode {
      find_key_or_close,
      find_separator,
      find_comma_or_close
    };
    mode m = mode::find_key_or_close;

    json key;
    while(!m_stream.eof()){
      skip_space_or_comment();
      const char c = m_stream[0];
      switch(m){
        case mode::find_key_or_close: {
          if(c == '}'){
            m_stream.next(1);
            j.set(std::move(obj));
            return;
          }  
          else if(is_blacket(c)) {
            deserialize_string(key);
            if(key.get<std::string>().empty()){
              throwError("object key is empty");
            }
            m = mode::find_separator;
          }
          else {
            throwError("syntax error");
          }
          break;
        }
        case mode::find_separator: {
          if(c == ':'){
            m_stream.next(1);
            json inner;
            deserialize(inner);
            obj.insert({key.get<std::string>(), inner});
            m = mode::find_comma_or_close;
          }
          else {
            throwError("syntax error");
          }
          break;
        }
        case mode::find_comma_or_close: {
          if(c == '}'){
            m_stream.next(1);
            j.set(std::move(obj));
            return;
          }  
          else if(c == ',') {
            m_stream.next(1);
            m = mode::find_key_or_close;
          }
          else {
            throwError("syntax error");
          }
          break;
        }
      }
    }
    throwError("illegal eof");
  }

  void deserialize_array(json& j)
  {
    m_stream.next(1); /** [ をスキップ */
    auto arr = json::array_type();
    while(!m_stream.eof()){
      skip_space_or_comment();
      const char c = m_stream[0];
      if(c == ']'){
        m_stream.next(1);
        j.set(std::move(arr));
        return;
      }
      else if(c == ','){
        if(arr.size() == 0){
          /* いきなりカンマ */
          throwError("syntax error");
        }
        else{
          m_stream.next(1);
        }
      }
      else{
        json inner;
        deserialize(inner);
        arr.push_back(inner);
      }
    }
    throwError("illegal eof");
  }

  void deserialize_string(json& j)
  {
    m_stream.next(1); /** blacket をスキップ */
    std::stringstream ss;
    while(!m_stream.eof()){
      const char c = m_stream[0];
      if(is_blacket(c)){
        m_stream.next(1);
        j.set(ss.str());
        return;
      }
      else if(c == '\\'){
        unescape(ss);
      }
      else if(c == '\r' || c == '\n' || c == '\b' || c == '\f' || c == '\t' ){
        throwError("string literal cannot contain control codes.");
      }
      else{
        m_stream.next(1);
        ss << c;
      }
    }
    throwError("illegal eof");
  }

  void deserialize_number(json& j)
  {
    std::stringstream ss;
    bool bFloat = false;
    while(!m_stream.eof()){
      const char c = m_stream[0];

      if(!is_number_parts(c)) break;
      m_stream.next(1);
      if(c == '.' || c == 'e' || c == 'E'){
        bFloat = true;
      }
      ss << c;
    }
    auto&& s = ss.str();
    try{
      if(bFloat){
        auto v = std::stod(ss.str().c_str());
        j.set(v);
      }
      else{
        auto v = std::stoll(ss.str().c_str());
        j.set(v);
      }
    }
    catch(std::exception& e){
      std::stringstream errss;
      errss << "cannot convert to number : \"" << s << "\" (" << e.what() << ")" << std::endl;
      throwError(errss.str());
    }
  }

  void check_value(const std::string& s)
  {
    if(s == m_stream.str(s.size())){
      m_stream.next(s.size());
    }
    else{
      throwError("syntax error");
    }
  } 

  void skip_space_or_comment() {
    while(!m_stream.eof()){
      const char c1 = m_stream[0];
      const char c2 = m_stream[1];
      if(c1 == '\r' && c2 == '\n'){
        m_stream.next(2);
        m_stream.newLine();
      }
      else if(c1 == '\r' || c1 == '\n') {
        m_stream.next(1);
        m_stream.newLine();
      }
      else if(std::isspace(c1)){
        m_stream.next(1);
      }
      else if(c1 == '/' && c2 == '*'){
        skip_block_comment();
      }
      else if(c1 == '/' && c2 == '/'){
        skip_line_comment();
      }
      else {
        break;
      }
    }
  }

  void skip_block_comment()
  {
    m_stream.next(2); /** 開始マークをスキップ */
    while(!m_stream.eof()) {
      const char c1 = m_stream[0];
      const char c2 = m_stream[1];
      if(c1 == '\r' && c2 == '\n'){
        m_stream.next(2);
        m_stream.newLine();
      }
      else if(c1 == '\r' || c1 == '\n') {
        m_stream.next(1);
        m_stream.newLine();
      }
      else if(c1 == '*' && c2 == '/'){
        m_stream.next(2);
        break;
      }
      m_stream.next(1);
    }
  }

  void skip_line_comment()
  {
    m_stream.next(2); /** 開始マークをスキップ */
    while(!m_stream.eof()) {
      const char c1 = m_stream[0];
      const char c2 = m_stream[1];
      if(c1 == '\r' && c2 == '\n'){
        m_stream.next(2);
        break;
      }
      if(c1 == '\r' || c1 == '\n') {
        m_stream.next(1);
        break;
      }
      m_stream.next(1);
    }
  }

  void deserialize(json& j)
  {
    while(!m_stream.eof()){
      skip_space_or_comment();
      const char c = m_stream[0];
      if(c == '{'){
        deserialize_object(j);
        return;
      }
      else if(c == '['){
        deserialize_array(j);
        return;
      }
      else if(c == 't'){
        check_value("true");
        j.set(true);
        return;
      }
      else if(c == 'f'){
        check_value("false");
        j.set(false);
        return;
      }
      else if(c == 'n'){
        check_value("null");
        j.set(nullptr);
        return;
      }
      else if(is_blacket(c)){
        deserialize_string(j);
        return;
      }
      else if(is_number_parts(c)){
        deserialize_number(j);
        return;
      }
      else{
        throwError("syntax error");
      }
    }
    throwError("illegal eof");
  }

public:
  deserializer(std::istream& stream) :
    m_stream(stream)
  {
  }

  ~deserializer() = default;

  json execute() {
    json j;
    deserialize(j);
    return j;
  }

  void execute(json& j) {
    deserialize(j);
  }
};

} /** namespace cppjson */
#endif /* !defined(__cppjson_h_deserializer__) */
