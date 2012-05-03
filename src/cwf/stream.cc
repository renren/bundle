#include "cwf/stream.h"

#include<boost/tokenizer.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_array.hpp>

#include "base3/common.h"
#include "base3/stringencode.h"

#include "3rdparty/libfcgi/fcgiapp.h"

namespace cwf {

/* 
GET /en-US/firefox/headlines.xml HTTP/1.1
Host: fxfeeds.mozilla.com
User-Agent: Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.9.1.4) Gecko/20091016 Firefox/3.5.4 GTB5
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,* / *;q=0.8
Accept-Language: en-us,en;q=0.5
Accept-Encoding: gzip,deflate
Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7
Keep-Alive: 300
Connection: keep-alive
X-Moz: livebookmarks
Cookie: __utma=183859642.141409536.1246462943.1256907184.1256964634.3; __utmz=183859642.1246462943.1.1.utmccn=(direct)|utmcsr=(direct)|utmcmd=(none); s_vi=[CS]v1|4A4B83DF00003166-A02085D000000E3[CE]
Cache-Control: max-age=0


HTTP/1.x 302 Found
Server: Apache/2.2.3 (Red Hat)
Content-Type: text/html; charset=iso-8859-1
Date: Thu, 05 Nov 2009 15:16:00 GMT
Location: http://fxfeeds.mozilla.com/firefox/headlines.xml
Keep-Alive: timeout=20, max=854
Expires: Thu, 05 Nov 2010 15:16:06 GMT
Accept-Ranges: bytes
Connection: Keep-Alive
X-Cache-Info: caching
Content-Length: 317

HTTP/1.x 200 OK
Content-Type: text/xml; charset=utf-8
Expires: Thu, 05 Nov 2009 15:16:00 GMT
Content-Encoding: gzip
Transfer-Encoding: chunked
Date: Thu, 05 Nov 2009 15:16:00 GMT
Cache-Control: private, max-age=0
X-Content-Type-Options: nosniff
X-XSS-Protection: 0
Server: GFE/2.0

HTTP/1.x 200 OK
Cache-Control: private, max-age=0
Date: Thu, 05 Nov 2009 15:16:26 GMT
Expires: -1
Content-Type: text/html; charset=ISO-8859-1
Server: gws
Content-Length: 0
X-XSS-Protection: 0

HTTP/1.x 200 OK
Content-Type: image/x-icon
Last-Modified: Fri, 09 Jan 2009 16:35:02 GMT
Date: Thu, 05 Nov 2009 15:16:26 GMT
Expires: Fri, 05 Nov 2010 15:16:26 GMT
Server: gws
Content-Length: 1150
X-XSS-Protection: 0
Cache-Control: public, max-age=31536000
Age: 0

HTTP/1.x 200 OK
Content-Type: text/html; charset=UTF-8
Date: Thu, 05 Nov 2009 15:20:25 GMT
Pragma: no-cache
Expires: Fri, 01 Jan 1990 00:00:00 GMT
Cache-Control: no-cache, no-store, must-revalidate
X-Content-Type-Options: nosniff
Server: igfe
X-XSS-Protection: 0
Content-Encoding: gzip
Transfer-Encoding: chunked

# nginx conf
location ~ \.php$ {
  root           html;
  fastcgi_pass   127.0.0.1:3000;
  fastcgi_index  index.php;
  fastcgi_param  SCRIPT_FILENAME  /scripts$fastcgi_script_name;
  include        fastcgi_params;
}
*/

const std::string kEmptyString;

bool Request::Init(FCGX_Stream* in, FCGX_ParamArray envp) {
    // 1 enum envp => headers
    // 2 url ...
    // 3 parse cookie

  static char sep('=');

  char** p = envp;
  for (; *p; ++p) {
    std::string str(*p);
    
    // 不合法的env
    std::string::size_type pos_equal = str.find_first_of(sep);
    if (pos_equal == std::string::npos)
      continue;

    std::string key = str.substr(0, pos_equal);
    std::string value = str.substr(pos_equal + 1);

    // 必须验证，有些 Header 会出现为空的情形
    // 玉伟的某用户出现此状况
    if (value.empty())
      continue;

    // Fastcgi 会在 Header 前加上 HTTP_
    // HTTP_XXX ==> Header XXX
    static const std::string HTTP_("HTTP_");
    if (boost::starts_with(key, HTTP_))
      header_[key.substr(HTTP_.size())] = value;

    // 2 
    else if(key == "QUERY_STRING")
      query_string_ = value;

    else if(key == "REQUEST_METHOD")
      FromString(method_, value);

    else if(key == "REQUEST_URI")
      url_ = value;
    else
      header_[key] = value;
  }
/*
"FCGI_ROLE=RESPONDER"	
"SCRIPT_FILENAME=/scripts/a.php"	
"QUERY_STRING=key=value"	
"REQUEST_METHOD=GET"	
"REQUEST_URI=/a.php?key=value"	

"CONTENT_TYPE="	
"CONTENT_LENGTH="	
"SCRIPT_NAME=/a.php"	

"DOCUMENT_URI=/a.php"	
"DOCUMENT_ROOT=C:\nginx-0.8.22/html"	
"SERVER_PROTOCOL=HTTP/1.1"	
"GATEWAY_INTERFACE=CGI/1.1"	
"SERVER_SOFTWARE=nginx/0.8.22"	
"REMOTE_ADDR=127.0.0.1"	
"REMOTE_PORT=2490"	
"SERVER_ADDR=127.0.0.1"	
"SERVER_PORT=80"	
"SERVER_NAME=localhost"	
"REDIRECT_STATUS=200"	

"HTTP_HOST=localhost"	
"HTTP_USER_AGENT=Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.9.1.4) Gecko/20091016 Firefox/3.5.4 GTB5"	
"HTTP_ACCEPT=text/html,application/xhtml+xml,application/xml;q=0.9,* / *;q=0.8"	
"HTTP_ACCEPT_LANGUAGE=en-us,en;q=0.5"	
"HTTP_ACCEPT_ENCODING=gzip,deflate"	
"HTTP_ACCEPT_CHARSET=ISO-8859-1,utf-8;q=0.7,*;q=0.7"	
"HTTP_KEEP_ALIVE=300"	
"HTTP_CONNECTION=keep-alive"	
"HTTP_CACHE_CONTROL=max-age=0"	
*/

  host_ = header("HOST");
  cookie_.Parse(header("COOKIE"));

  // query string to StringMap
  if (!query_string_.empty()) {
    query_ = ParseQuery(query_string_);
  }

  ASSERT(method_ >= HV_GET && method_<= HV_LAST);

  ASSERT(!url_.empty());
  ASSERT(!host_.empty());

  if (method_ == HV_POST) {
    std::string data;
    {
      size_t readed = 0;
      char buf[2048];
      do {
        readed = FCGX_GetStr(buf, sizeof(buf), in);
        if (readed)
          data.append(buf, readed);

      } while (readed == sizeof(buf));
    }

    if (!data.empty()) {
      // Content-Type: text/html; charset=ISO-8859-4
      std::string type = header("CONTENT_TYPE");
      // DCHECK(!type.empty());

      // application/x-www-form-urlencoded; utf8
      std::string part_type;
      std::string::size_type pos = type.find_first_of(';');
      if (std::string::npos != pos)
        part_type = type.substr(0, pos);
      else
        part_type = type;

      if (part_type == "application/x-www-form-urlencoded") {
        form_ = ParseQuery(data);
      } else {
        if (part_type == "multipart/form-data") {
          DispositionArrayType arr = ParseMultipartForm(type, data);
          for (DispositionArrayType::const_iterator i=arr.begin(); 
            i!=arr.end(); ++i) {
              const FormDisposition & afd = *i;
              
              if (afd.filename.empty()) // ugly: 如果为空，是普通字段, 合并至 form_
                form_.insert(std::make_pair(afd.name, afd.data));
              else
                file_array_.push_back(afd);
          }
        }
        else if (part_type == "application/octet-stream") {
          FormDisposition fd = ParseOctetStream(data);
          if (!fd.data.empty()) {
            fd.name = "noname"; // TODO:
            file_array_.push_back(fd);
          }
        }
      }
    }
  }

  if (!query_string_.empty())
    ASSERT(!query_.empty());

  ASSERT(!header_.empty());

  if (url_.empty() || host_.empty() || header_.empty())
    return false;

  return true;
}

size_t url_decode(const char* source, size_t srclen, std::string& text) {
  boost::scoped_array<char> buf(new char[srclen + 1]);
  size_t ret = base::url_decode(buf.get(), srclen + 1, source, srclen);
  text = buf.get();
  return ret;
}

StringMap Request::ParseQuery(const std::string& text) {
  using namespace std;
  typedef vector<string> split_vector_type;

  StringMap ret;

  split_vector_type equal_array; // a=b
  boost::split(equal_array, text, boost::is_any_of("&"));

  for (split_vector_type::const_iterator i=equal_array.begin();
    i != equal_array.end(); ++i) {
    std::string::size_type pos_equal = i->find_first_of('=');
    if (pos_equal != std::string::npos) {
      std::string key = i->substr(0, pos_equal);
      std::string value = i->substr(pos_equal + 1);
      std::string d;
      url_decode(value.c_str(), value.size(), d);
      ret.insert(std::make_pair(key, d));
    }
  }
  return ret;
}

std::string RemoveQuote(const std::string & text) {
  if (text.size() >= 2 && text[0] == '"' && text[text.size() - 1] == '"') {
    return text.substr(1, text.size() - 2);
  }
  return text;
}

Request::FormDisposition ParseDisposition(const std::string & text
                               , std::string::size_type start
                               , std::string::size_type end) {
  Request::FormDisposition afd;
  
  std::string::size_type header_start = text.find("Content-Disposition", start);
  if (header_start != start)
    return afd;

// Content-Disposition 格式见 RFC1806
// http://tools.ietf.org/html/rfc1806
// take a look at stream_test.cc
  
  std::string::size_type header_end = text.find("\r\n\r\n", start);
  if (std::string::npos == header_end || header_end > end)
    return afd;

  // split by \r\n
  std::string::size_type p = header_start;
  do {
    std::string::size_type cr = text.find("\r\n", p);
    // [p, cr) name: value; parameter-name="parameter value"

    // ': '
    std::string::size_type name_end = text.find(": ", p);
    if (std::string::npos != name_end && name_end < cr) {
      std::string name = text.substr(p, name_end - p);
      if ("Content-Type" == name)
        afd.content_type = text.substr(name_end + 2, cr - (name_end + 2));
      else if ("Content-Disposition" == name) {
        std::string::size_type par_pos = name_end;
        do {
          std::string::size_type par_start = text.find("; ", par_pos);
          if (par_start > cr - 5) // 其后至少4字符，不设置可能导致死循环
             break;
          std::string::size_type par_end = text.find("=", par_start);
          if (par_end > cr - 2) // 其后至少1字符
             break;
          std::string::size_type value_end = text.find("; ", par_end);
          if (value_end > cr)
            value_end = cr;

          std::string par_name = text.substr(par_start + 2, par_end - (par_start + 2));
          std::string par_value = text.substr(par_end + 1, value_end - (par_end+1));
          par_name = RemoveQuote(par_name);
          par_value = RemoveQuote(par_value);

          if ("name" == par_name)
            afd.name = par_value;
          else if ("filename" == par_name)
            afd.filename = par_value;

          par_pos = value_end;
        } while (par_pos < cr);
      }
    }

    p = cr + 2;
  } while (p < header_end);

  // TODO: more check
  // [header_end + 4, end - 2)
  afd.data = text.substr(header_end + 4, end - (header_end + 4) - 2);

  return afd;
}

Request::DispositionArrayType Request::ParseMultipartForm(const std::string & content_type
                                                  , const std::string& text) {

  // http://tools.ietf.org/html/rfc1867
  std::string boundary("--");
  std::string::size_type pos = content_type.find("boundary=");
  if (std::string::npos != pos) {
    // content_type[pos+1] 有可能为 "
    if ('"' == content_type[pos+1]) {
      // TODO:
    }
    else {
      boundary += content_type.substr(pos + std::string("boundary=").size());
    }
  }

  Request::DispositionArrayType result;

  // element split by boundary
  pos = 0;
  while (true) {
    std::string::size_type boundary_start = text.find(boundary, pos);
    if (std::string::npos == boundary_start)
      break;

    boundary_start += boundary.size();

    // 略过 \r\n
    if (text.size() < boundary_start + 2 && (
      '\r' != text[boundary_start] || '\n' != text[boundary_start + 1])) {
        break;
    }
    boundary_start += 2;

    // find this boundary's end
    std::string::size_type boundary_end = text.find(boundary, boundary_start);
    if (std::string::npos == boundary_end)
      break;

    FormDisposition afd = ParseDisposition(text, boundary_start, boundary_end);
    result.push_back(afd);

    pos = boundary_end;
  }

  return result;
}

Request::FormDisposition Request::ParseOctetStream(const std::string& text) {
  return Request::FormDisposition();
}

int Request::query(const char * key, int default_value) const {
  const std::string& s = query(key);
  if (s.empty())
    return default_value;

  try {
    return boost::lexical_cast<int>(s);
  } catch(...) {
    return default_value;
  }
}

int Request::form(const char * key, int default_value) const {
  const std::string& s = form(key);
  if (s.empty())
    return default_value;

  try {
    return boost::lexical_cast<int>(s);
  } catch(...) {
    return default_value;
  }
}

int Request::get(const char * key, int default_value) const {
  const std::string* s = &query(key);
  if (s->empty()) {
    s = &form(key);
    if (s->empty())
      return default_value;
  }
  
  try {
    return boost::lexical_cast<int>(*s);
  } catch(...) {
    return default_value;
  }
}

bool Response::OutputHeader() {
  std::ostringstream ostem;
  header_.Write(ostem);

  std::string r = ostem.str();
  FCGX_PutStr(r.c_str(), r.size(), out_);
  return true;
}

void Response::WriteRaw(const std::string & text) {
  FCGX_PutStr(text.data(), text.size(), out_);
}

void Response::WriteRaw(const char* buf, size_t size) {
  FCGX_PutStr(buf, size, out_);
}

void Response::WriteRawWithHeader(const char* buf, size_t size) {
  std::ostringstream ostem;
  header_.Write(ostem);
  ostem.write(buf, size);

  std::string r = ostem.str();
  FCGX_PutStr(r.c_str(), r.size(), out_);
}

}
