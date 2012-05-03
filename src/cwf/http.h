#ifndef CWF_HTTP_HPP__
#define CWF_HTTP_HPP__

#include <string>
#include <vector>
#include <boost/cstdint.hpp>

namespace cwf {

/// Standard HTTP status codes
   /**
    * See http://tools.ietf.org/html/rfc2616#section-10
    */
enum HttpStatusCode {
  /// informational codes
  HC_CONTINUE                      = 100, // note the trailing underscore
  HC_SWITCHING_PROTOCOLS,
  HC_PROCESSING,

  /// success codes
  HC_OK                             = 200,
  HC_CREATED,
  HC_ACCEPTED,
  HC_NON_AUTHORATIVE_INFORMATION,
  HC_NO_CONTENT,
  HC_RESET_CONTENT,
  HC_PARTIAL_CONTENT,
  HC_MULTI_STATUS,

  /// redirect codes
  HC_MULTIPLE_CHOICES               = 300,
  HC_MOVED_PERMANENTLY,
  HC_FOUND,
  HC_SEE_OTHER,
  HC_NOT_MODIFIED,
  HC_USE_PROXY,
  HC_SWITCH_PROXY,
  HC_TEMPORARY_REDIRECT,

  /// domain error codes
  HC_BAD_REQUEST                    = 400,
  HC_UNAUTHORIZED,
  HC_PAYMENT_REQUIRED,
  HC_FORBIDDEN,
  HC_NOT_FOUND,
  HC_METHOD_NOT_ALLOWED,
  HC_NOT_ACCEPTABLE,
  HC_PROXY_AUTHENTICATION_REQUIRED,
  HC_REQUEST_TIMEOUT,
  HC_CONFLICT,
  HC_GONE,
  HC_LENGTH_REQUIRED,
  HC_PRECONDITION_FAILED,
  HC_REQUEST_ENTITY_TOO_LARGE,
  HC_REQUEST_URI_TOO_LONG,
  HC_UNSUPPORTED_MEDIA_TYPE,
  HC_REQUEST_RANGE_NOT_SATISFIABLE,
  HC_EXPECTATION_FAILED,
  HC_UNPROCESSABLE_ENTITY           = 422,
  HC_LOCKED,
  HC_FAILED_DEPENDENCY,
  HC_UNORDERED_COLLECTION,
  HC_UPGRADE_REQUIRED,
  HC_RETRY_WITH                     = 449,

  /// internal error codes
  HC_INTERNAL_SERVER_ERROR          = 500,
  HC_NOT_IMPLEMENTED,
  HC_BAD_GATEWAY,
  HC_SERVICE_UNAVAILABLE,
  HC_GATEWAY_TIMEOUT,
  HC_HTTP_VERSION_NOT_SUPPORTED,
  HC_INSUFFICIENT_STORAGE,
  HC_BANDWIDTH_LIMIT_EXCEEDED       = 509
};

enum HttpVersion {
  HVER_1_0, HVER_1_1,
  HVER_X,
  HVER_LAST = HVER_X  
};

enum HttpVerb {
  HV_GET, HV_POST, HV_PUT, HV_DELETE, HV_CONNECT, HV_HEAD,
  HV_LAST = HV_HEAD
};

enum HttpHeader {
  HH_ACCEPT,
  HH_ACCEPT_CHARSET,
  HH_ACCEPT_ENCODING,
  HH_ACCEPT_LANGUAGE,
  HH_AGE,
  HH_CACHE_CONTROL,
  HH_CONNECTION,
  HH_CONTENT_LENGTH,
  HH_CONTENT_RANGE,
  HH_CONTENT_TYPE,
  HH_COOKIE,
  HH_DATE,
  HH_ETAG,
  HH_EXPIRES,
  HH_HOST,
  HH_IF_MODIFIED_SINCE,
  HH_IF_NONE_MATCH,
  HH_KEEP_ALIVE,
  HH_LAST_MODIFIED,
  HH_LOCATION,
  HH_PROXY_AUTHENTICATE,
  HH_PROXY_AUTHORIZATION,
  HH_PROXY_CONNECTION,
  HH_RANGE,
  HH_SERVER,
  HH_SET_COOKIE,
  HH_TE,
  HH_TRAILERS,
  HH_TRANSFER_ENCODING,
  HH_UPGRADE,
  HH_USER_AGENT,
  HH_WWW_AUTHENTICATE,
  HH_LAST = HH_WWW_AUTHENTICATE
};

const char* ToString(HttpVersion version);
bool FromString(HttpVersion& version, const std::string& str);

const char* ToString(HttpVerb verb);
bool FromString(HttpVerb& verb, const std::string& str);

const char* ToString(HttpHeader header);
bool FromString(HttpHeader& header, const std::string& str);

typedef boost::uint32_t uint32;

inline bool HttpCodeIsInformational(uint32 code) { return ((code / 100) == 1); }
inline bool HttpCodeIsSuccessful(uint32 code)    { return ((code / 100) == 2); }
inline bool HttpCodeIsRedirection(uint32 code)   { return ((code / 100) == 3); }
inline bool HttpCodeIsClientError(uint32 code)   { return ((code / 100) == 4); }
inline bool HttpCodeIsServerError(uint32 code)   { return ((code / 100) == 5); }

bool HttpCodeHasBody(uint32 code);
bool HttpCodeIsCacheable(uint32 code);
bool HttpHeaderIsEndToEnd(HttpHeader header);
bool HttpHeaderIsCollapsible(HttpHeader header);

typedef std::pair<std::string, std::string> HttpAttribute;
typedef std::vector<HttpAttribute> HttpAttributeList;
void HttpParseAttributes(const char * data, size_t len, 
                         HttpAttributeList& attributes);
bool HttpHasAttribute(const HttpAttributeList& attributes,
                      const std::string& name,
                      std::string* value);
bool HttpHasNthAttribute(HttpAttributeList& attributes,
                         size_t index, 
                         std::string* name,
                         std::string* value);

// Convert RFC1123 date (DoW, DD Mon YYYY HH:MM:SS TZ) to unix timestamp
bool HttpDateToSeconds(const std::string& date, unsigned long* seconds);

} // cwf

#endif // CWF_HTTP_HPP__
