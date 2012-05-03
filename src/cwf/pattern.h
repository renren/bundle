#ifndef CWF_PATTERN_H__
#define CWF_PATTERN_H__

#include <string>
#include <boost/algorithm/string/predicate.hpp>
#if 0
#include <boost/regex.hpp>
#include <boost/algorithm/string/regex.hpp>
#endif

namespace cwf {

struct Pattern {
  virtual ~Pattern() {}
  virtual bool Match(std::string const& url) const = 0;
};

struct StringPattern : public Pattern {
  StringPattern(const std::string& text) : text_(text) {}

  bool Match(std::string const& url) const {
    return boost::starts_with(url, text_);
  }

private:
  std::string text_;
};

struct AnyPattern : public Pattern {
  bool Match(std::string const&) const {
    return true;
  }
};

#if 0
struct RegexPattern : public Pattern {
  RegexPattern(const std::string& regex) : rx_(regex) {}

  bool Match(std::string const& url) const {
    boost::iterator_range<std::string::const_iterator> r 
      = boost::find_regex(url, rx_);
    if (r = boost::iterator_range<std::string::const_iterator>(url.begin(), url.end()))
      return true;
    return false;
  }

private:
  boost::regex rx_;
};
#endif

}
#endif // CWF_PATTERN_H__
