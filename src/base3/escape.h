#ifndef BASE_BASE_ESCAPE_H__
#define BASE_BASE_ESCAPE_H__

#include <string>

namespace base {

// --- ESCAPE FUNCTORS
// Some commonly-used escape functors.

// Escapes < > " ' & <non-space whitespace> to &lt; &gt; &quot;
// &#39; &amp; <space>
struct HtmlEscape { std::string operator()(const std::string&) const; };

// Same as HtmlEscape but leaves all whitespace alone. Eg. for <pre>..</pre>
struct PreEscape { std::string operator()(const std::string&) const; };

// Escapes &nbsp; to &#160;
struct XmlEscape { std::string operator()(const std::string&) const; };

// Escapes " ' \ <CR> <LF> <BS> to \" \' \\ \r \n \b
struct JavascriptEscape { std::string operator()(const std::string&) const; };

// Escapes characters not in [0-9a-zA-Z.,_:*/~!()-] as %-prefixed hex.
// Space is encoded as a +.
struct UrlQueryEscape { std::string operator()(const std::string&) const; };

// Escapes " \ / <FF> <CR> <LF> <BS> <TAB> to \" \\ \/ \f \r \n \b \t
struct JsonEscape { std::string operator()(const std::string&) const; };
  
// ----------------------------------------------------------------------
// HtmlEscape
// PreEscape
// XMLEscape
// UrlQueryEscape
// JavascriptEscape
//    Escape functors that can be used by SetEscapedValue().
//    Each takes astd::string as input and gives astd::string as output.
// ----------------------------------------------------------------------

// Escapes < > " ' & <non-space whitespace> to &lt; &gt; &quot; &#39;
// &amp; <space>
inline std::string HtmlEscape::operator()(const std::string& in) const {
 std::string out;
  // we'll reserve some space in out to account for minimal escaping: say 12%
  out.reserve(in.size() + in.size()/8 + 16);
  for (int i = 0; i < in.length(); ++i) {
    switch (in[i]) {
      case '&': out += "&amp;"; break;
      case '"': out += "&quot;"; break;
      case '\'': out += "&#39;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '\r': case '\n': case '\v': case '\f':
      case '\t': out += " "; break;      // non-space whitespace
      default: out += in[i];
    }
  }
  return out;
}

// Escapes < > " ' & to &lt; &gt; &quot; &#39; &amp;
// (Same as HtmlEscape but leaves whitespace alone.)
inline std::string PreEscape::operator()(const std::string& in) const {
 std::string out;
  // we'll reserve some space in out to account for minimal escaping: say 12%
  out.reserve(in.size() + in.size()/8 + 16);
  for (int i = 0; i < in.length(); ++i) {
    switch (in[i]) {
      case '&': out += "&amp;"; break;
      case '"': out += "&quot;"; break;
      case '\'': out += "&#39;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      // All other whitespace we leave alone!
      default: out += in[i];
    }
  }
  return out;
}

// Escapes &nbsp; to &#160;
// TODO(csilvers): have this do something more useful, once all callers have
//                 been fixed.  Dunno what 'more useful' might be, yet.
inline std::string XmlEscape::operator()(const std::string& in) const {
 std::string out(in);

 std::string::size_type pos = 0;
  while (1) {
    pos = out.find("&nbsp;", pos);
    if ( pos ==std::string::npos )
      break;
    out.replace(pos, sizeof("&nbsp;")-1, "&#160;");
    pos += sizeof("&#160;")-1;   // start searching again after the replacement
  };
  return out;
}

// Escapes " ' \ <CR> <LF> <BS> to \" \' \\ \r \n \b
inline std::string JavascriptEscape::operator()(const std::string& in) const {
 std::string out;
  // we'll reserve some space in out to account for minimal escaping: say 1.5%
  out.reserve(in.size() + in.size()/64 + 2);
  for (int i = 0; i < in.length(); ++i) {
    switch (in[i]) {
      case '"': out += "\\\""; break;
      case '\'': out += "\\'"; break;
      case '\\': out += "\\\\"; break;
      case '\r': out += "\\r"; break;
      case '\n': out += "\\n"; break;
      case '\b': out += "\\b"; break;
      case '&': out += "\\x26"; break;
      case '<': out += "\\x3c"; break;
      case '>': out += "\\x3e"; break;
      case '=': out += "\\x3d"; break;
      default: out += in[i];
    }
  }
  return out;
}

inline std::string UrlQueryEscape::operator()(const std::string& in) const {
  // Everything not matching [0-9a-zA-Z.,_*/~!()-] is escaped.
  static unsigned long _safe_characters[8] = {
    0x00000000L, 0x03fff702L, 0x87fffffeL, 0x47fffffeL,
    0x00000000L, 0x00000000L, 0x00000000L, 0x00000000L
  };

  int max_string_length = in.size() * 3 + 1;
  char * out = new char[max_string_length];

  int i;
  int j;

  for (i = 0, j = 0; i < in.size(); i++) {
    unsigned char c = in[i];
    if (c == ' ') {
      out[j++] = '+';
    } else if ((_safe_characters[(c)>>5] & (1 << ((c) & 31)))) {
      out[j++] = c;
    } else {
      out[j++] = '%';
      out[j++] = ((c>>4) < 10 ? ((c>>4) + '0') : (((c>>4) - 10) + 'A'));
      out[j++] = ((c&0xf) < 10 ? ((c&0xf) + '0') : (((c&0xf) - 10) + 'A'));
    }
  }
  out[j++] = '\0';
 std::string ret(out);
  delete [] out;
  return ret;
}

// Escapes " / \ <BS> <FF> <CR> <LF> <TAB> to \" \/ \\ \b \f \r \n \t
inline std::string JsonEscape::operator()(const std::string& in) const {
 std::string out;
  // we'll reserve some space in out to account for minimal escaping: say 1.5%
  out.reserve(in.size() + in.size()/64 + 2);
  for (int i = 0; i < in.length(); ++i) {
    switch (in[i]) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '/': out += "\\/"; break;
      case '\b': out += "\\b"; break;
      case '\f': out += "\\f"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out += in[i];
    }
  }
  return out;
}

}
#endif // BASE_BASE_ESCAPE_H__
