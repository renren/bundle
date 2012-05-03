#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/lexical_cast.hpp>
#include <vector>
#include "base3/jdbcurl.h"

namespace base {

// http://www.petefreitag.com/articles/jdbc_urls/
// jdbc:mysql://[host][,failoverhost...][:port]/[database]
// jdbc:mysql://[host][,failoverhost...][:port]/[database][?propertyName1][=propertyValue1][&propertyName2][=propertyValue2]...

bool Parse(const std::string& text, JdbcDriver* dr) {
  const char* scheme = "jdbc:mysql://";
  if (!boost::starts_with(text, scheme))
    return false;

  size_t pos_start = std::string::traits_type::length(scheme);

  std::string::size_type pos_comma = text.find_first_of(',', pos_start);
  std::string::size_type pos_port = text.find_first_of(':', pos_start);
  std::string::size_type pos_slash = text.find_first_of('/', pos_start);

  // port
  if (pos_port == std::string::npos)
    dr->port = 3301;

  if (pos_port != std::string::npos && pos_slash > pos_port) {
    std::string port = text.substr(pos_port + 1, pos_slash - pos_port - 1);
    if (port.empty())
      return false;

    try {
      dr->port = boost::lexical_cast<int>(port);
      if (dr->port == 0)
        return false;
    } catch(...) {
      return false;
    }
  }

  // host failover
  if (pos_comma != std::string::npos) {
    // host,failover
    std::string::size_type end = pos_port == std::string::npos ? pos_slash : pos_port;
    dr->host = text.substr(pos_start, pos_comma - pos_start);
    dr->failoverhost = text.substr(pos_comma + 1, end - pos_comma - 1);
  } else {
    // host[:port]/
    std::string::size_type end = pos_port == std::string::npos ? pos_slash : pos_port;
    dr->host = text.substr(pos_start, end - pos_start);
  }

	// TODO: pos_slash == -1

  // database
  std::string::size_type pos_question = text.find_first_of('?', pos_slash);
  if (pos_question == std::string::npos) {
    dr->database = text.substr(pos_slash + 1);
    return true;
  }

  dr->database = text.substr(pos_slash + 1, pos_question - pos_slash - 1);

  // properties
  std::string p = text.substr(pos_question + 1);
  if (p.empty())
    return true;

  using namespace std;
  typedef vector<string> split_vector_type;

  split_vector_type SplitVec;
  boost::split(SplitVec, p, boost::is_any_of("&"));
  for (split_vector_type::const_iterator i = SplitVec.begin();
    i != SplitVec.end(); ++i) {
      // A=B
      std::string::size_type pos_equal = i->find_first_of('=');
      std::string k, v;
      if (pos_equal != std::string::npos) {
        k = i->substr(0, pos_equal);
        v = i->substr(pos_equal + 1);
      } else {
          k = i->substr(0);
          v = "";
      }
      if (k == "user") {
        dr->user = v;
        continue;
      }
      else if (k == "password") {
        dr->passwd = v;
        continue;
      }

      dr->properties.push_back(std::make_pair(k, v));
  }
  return true;
}

}
