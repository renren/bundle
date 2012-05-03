#ifndef BASE_BASE_JDBCURL_H__
#define BASE_BASE_JDBCURL_H__

#include <string>
#include <utility>
#include <list>

namespace base {

struct JdbcDriver { // JdbcDescriptor?
  // std::string schema; // TODO:
  std::string host, failoverhost;
  int port;
  std::string database;
  std::string user, passwd;

  typedef std::pair<std::string, std::string> PropertyType;
  std::list<PropertyType> properties;

  // TODO: 
  // std::string ToString() const;
};

// http://www.petefreitag.com/articles/jdbc_urls/
// jdbc:mysql://[host][,failoverhost...][:port]/[database]
// jdbc:mysql://[host][,failoverhost...][:port]/[database][?propertyName1][=propertyValue1][&propertyName2][=propertyValue2]...

bool Parse(const std::string& text, JdbcDriver* dr);

}
#endif // BASE_BASE_JDBCURL_H__
