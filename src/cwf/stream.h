#ifndef CWF_STREAM_H__
#define CWF_STREAM_H__

#include <sstream>
#include <list>
#include <vector>
#include <map>
#include <boost/foreach.hpp>
// #include "ctemplate/template_emitter.h"
#include "cwf/site.h"
#include "cwf/http.h"
#include "cwf/cookie.h"

// forward declare, from fcgiapp.h
struct FCGX_Stream;
typedef char **FCGX_ParamArray;

namespace cwf {

typedef std::map<std::string, std::string> StringMap;

// used in stream.h
extern const std::string kEmptyString;

struct Request {
  Request() : user_(0), method_(HV_GET) {}

  // !! 必须保证只有一个线程读
  bool Init(FCGX_Stream* in, FCGX_ParamArray envp);

  // bool is_authorize() const { return false; }
  User* user() const {
    return user_;
  }
  void set_user(User* user) {
    user_ = user;
  }

  HttpVerb method() const {
    return method_;
  }

  const std::string& host() const {
    return host_;
  }

  // url
  const std::string& url() const {
    return url_;
  }
  const std::string& raw_query() const {
    return query_string_;
  }
  const std::string& query(const char* key) const {
    StringMap::const_iterator i = query_.find(std::string(key));
    if (query_.end() != i)
      return i->second;
    return kEmptyString;
  }

  int query(const char*key, int default_value) const;

  const std::string& form(const char* key) const {
    StringMap::const_iterator i = form_.find(std::string(key));
    if (form_.end() != i)
      return i->second;
    return kEmptyString;
  }

  const StringMap& querys() const {
    return query_;
  }
  const StringMap& forms() const {
    return form_;
  }

  int form(const char*key, int default_value) const;

  const std::string& get(const char* key) const {
    const std::string & ret = query(key);
    if (ret.empty())
      return form(key);
    return ret;
  }

  int get(const char*key, int default_value) const;

  // for multipart/form-data
  struct FormDisposition {
    std::string name;
    std::string filename;
    std::string content_type;
    std::string data;
  };
  typedef std::vector<FormDisposition> DispositionArrayType;

  const DispositionArrayType & files()  const {
    return file_array_;
  }

  // header
  const StringMap& header() const {
    return header_;
  }
  const std::string& header(const char* key) const {
    StringMap::const_iterator i = header_.find(std::string(key));
    if (header_.end() != i)
      return i->second;
    return kEmptyString;
  }

  Cookie& cookie() { return cookie_; }
  Cookie const& cookie() const { return cookie_; }

  ~Request() {
    delete user_;
  }

  static StringMap ParseQuery(const std::string& text);
  static DispositionArrayType ParseMultipartForm(const std::string & content_type
                                      , const std::string& text);
  static FormDisposition ParseOctetStream(const std::string& text);

  std::string host_, url_;
  User* user_;
  Cookie cookie_;
  StringMap query_; // TODO: 大小写不敏感
  StringMap header_;
  std::string query_string_;

  StringMap form_;
  DispositionArrayType file_array_;
  HttpVerb method_;
};

// for write purpose
// method ?
class Header {
public:
  void Add(HttpHeader hh, std::string const& value) {
    list_.push_back(
      std::make_pair(hh, value)
      );
  }
  void set_status_code(HttpStatusCode hc, const std::string & message) {
    hc_ = hc;
    message_ = message;
  }

  HttpStatusCode status_code() const {
    return hc_;
  }

  void Write(std::ostream& ostem) const {
    static const std::string CRLF("\r\n");
    ostem << "HTTP/" << ToString(HVER_X) << " " << hc_ << " "  << message_ << CRLF;
    BOOST_FOREACH(const item_type& i, list_) {
      ostem << ToString(i.first) << ": " << i.second << CRLF;
    }
    ostem << CRLF;
  }

private:
  typedef std::pair<HttpHeader, std::string> item_type;
  typedef std::list<item_type> list_type;
  list_type list_;

  // HttpVersion version_;
  HttpStatusCode hc_;
  std::string message_;
};

class Response /*: public ctemplate::ExpandEmitter*/ {
public:
  // TODO: 实现一个gzip版 Emitter
  virtual void Emit(char c) {
    stem_ << c;
  }
  virtual void Emit(const std::string& s) {
    stem_ << s;
  }
  virtual void Emit(const char* s) {
    stem_ << s;
  }
  virtual void Emit(const char* s, size_t slen) {
    stem_.write(s, static_cast<std::streamsize>(slen));
  }

  // 
  Header& header() {
    return header_;
  }
  const Header& header() const {
    return header_;
  }

  bool OutputHeader();

  void WriteRaw(const std::string &);
  void WriteRaw(const char*, size_t);

  void WriteRawWithHeader(const char*, size_t);

  // 貌似写入 out, err 时需要加锁
  Response(FCGX_Stream* out, FCGX_Stream* err) 
    : out_(out), err_(err) {}

  std::string str() const {
    return stem_.str();
  }

private:
  std::ostringstream stem_;
  Header header_;
  FCGX_Stream* out_, * err_;
};

}
#endif // CWF_STREAM_H__
