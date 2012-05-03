// Copyright 2009 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


// Url class contains all the components of a URL. Besides the url component
// variables, this class also provides method to parse url, methods to
// encode/decode URL string, and method to calculate url finger print.

#ifndef BASE3_URL_H__
#define BASE3_URL_H__

#include <string>
#include <cstring>

#include "base3/basictypes.h"

typedef uint64 UrlFprint;

class Url {
 public:
  // The url is expected to be "path?query"
  // In windows, when finger print is calculated, the path's case is ignored.
  static UrlFprint FingerPrint(const char* url);

  // Acts as the non-static method.
  static bool Validate(const char* url);

  
  // Escape single url component according to RFC2396.
  // These methods are used to  single URL component. So any character in "comp"
  // string is treated is a regular character.
  // Note, all following escape method doesn't clear dest before putting value
  // into it.
  static void EscapeUrlComponent(const char* comp, std::string* dest) {
    EscapeUrlComponent(comp, static_cast<int>(strlen(comp)), dest);
  }
  static void EscapeUrlComponent(const char* comp, int len, std::string* dest);

  static bool UnescapeUrlComponent(const char* comp, std::string* dest) {
    return UnescapeUrlComponent(comp, static_cast<int>(strlen(comp)), dest);
  }
  static bool UnescapeUrlComponent(const char* comp, int len, std::string* dest);

  // Unescape a whole URL.
  static bool UnescapeUrl(const char* url, std::string* dest) {
    return UnescapeUrlComponent(url, dest);
  }
  // NO IMPLEMENTATION YET.
  static bool EscapeUrl(const char* url, std::string* dest);

  // Check whether all chars contained are valid.
  static bool ValidateUrlChars(const char* url);

  Url();
  Url(const char *url);

  // Clear content of this object.
  void Clear();

  // Parse a url string.
  // Url format: http://user@password:host.name:port/path?parameter
  // Example: www.google.com
  //          http://www.google.com
  //          https://www.google.com:100
  //          :100
  //          /
  //          /test.html
  //          /test.html?search=abc
  //          https://www.google.com:100/test.html?search=abc
  // See unit test for more informatio nabout the supported url formats.
  bool Parse(const char *url);

  // Validate if current url is a valid url.
  bool Validate() const;

  // Return true if host url are exactly same.
  // Assume host are already converted to lower case.
  bool HostUrlEqual(const char *host, int port) const {
    return host_ == host && port_ == port;
  }

  // Return true if url are exactly same.
  bool operator == (const Url &url) const {
    return Equal(url);
  }

  // Return true if url are not exactly same.
  bool operator != (const Url &url) const {
    return !Equal(url);
  }

  // Get/set data members.
  const bool http() const { return http_; }
  const std::string& user() const { return user_; }
  const std::string& password() const { return password_; }
  const std::string& host() const { return host_; }
  const int port() const { return port_; }
  const std::string& path() const { return path_; }
  const std::string& parameter() const { return parameter_; }

  const std::string& url() const { return url_; }
  const std::string& host_url() const { return host_url_; }
  const std::string& path_url() const { return path_url_; }

 private:
  // Update all url components from full url string: this->url_.
  void UpdateUrl();

  // Return true if url are exactly same.
  bool Equal(const Url &url) const {
    return url_ == url.url();
  }

  // whether the protocal is http.
  bool          http_;
  std::string   user_;
  std::string   password_;
  std::string   host_;
  int           port_;
  std::string   path_;
  std::string   parameter_;

  // Full url string
  // A full url looks like:
  // http://user:password@host:port/path?parameter
  std::string   url_;

  // Only host url (without path, user_, password_)
  std::string   host_url_;

  // Only path + parameter
  std::string   path_url_;
};

#endif  // BASE3_URL_H__
