#ifndef CWF_COOKIE_H__
#define CWF_COOKIE_H__

#include <string>
#include <map>
#include <vector>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace cwf {

extern const std::string kEmptyString;

class Cookie {
public:
  struct Item {
    std::string name, value;
    // std::string comment;
    std::string domain, path;
    std::string expires;
    unsigned maxage : 31;
    unsigned secure : 1;
    std::string version;

    // SID=;Domain=.google.com;Path=/;Expires=Tue, 03-Sep-2019 11:25:23 GMT
    std::string text() const {
      std::string ret;
      ret += name + "=" + value;
      if (!domain.empty()) {
        ret.append(";Domain=");
        ret.append(domain);
      }

      if (!path.empty()) {
        ret.append(";Path=");
        ret.append(path);
      }

      if (!expires.empty()) {
        ret.append(";Expires=");
        ret.append(expires);
      }

      return ret;
    }
  };

  Cookie() {}
  Cookie(const std::string& text) {
    Parse(text);
  }

  // TODO: use HttpParseAttributes
  bool Parse(const std::string& text) {
    // 1 Cookie: a=b;
    // 2 a=b;
    std::string t;
    const char* pre = "Cookie: ";
    if (boost::starts_with(text, pre))
      t = text.substr(std::string::traits_type::length(pre));
    else
      t = text;

    // split "; "
    using namespace std;
    typedef vector<string> split_vector_type;

    split_vector_type SplitVec; // #2: Search for tokens
    boost::split( SplitVec, t, boost::is_any_of("; *"));

    // hello abc-*-ABC-*-aBc goodbye
    // -* SplitVec == { "hello abc","ABC","aBc goodbye" }

    for (split_vector_type::const_iterator i = SplitVec.begin();
      i != SplitVec.end(); ++i) {
      // A=B
      std::string::size_type pos_equal = i->find_first_of('=');
      if (pos_equal != std::string::npos) {
        Add(i->substr(0, pos_equal), i->substr(pos_equal + 1));
      }
    }
    return true;
  }

  const std::string& Get(const char* name) const {
    cookie_type::const_iterator i = cookie_.find(name);
    if (i != cookie_.end()) {
      return i->second.value;
    }
    return kEmptyString;
  }

  void Add(const std::string& name, const std::string& val) {
    std::pair<cookie_type::iterator, bool> p = cookie_.insert(
      std::make_pair(name, Item()));
    p.first->second.value = val; // ugly
  }

  // name, value, domain, path, maxage, secure, version
  Item& Add(const std::string& name) {
    std::pair<cookie_type::iterator, bool> p = cookie_.insert(
      std::make_pair(name, Item()));
    return p.first->second; // ugly
  }

  std::string text() const {
    std::string ret;
    for (cookie_type::const_iterator i=cookie_.begin(); 
      i!=cookie_.end(); ++i) {
        const Item& item = i->second;
        ret += item.text();
    }
    return ret;
  }

  // Cookie: GRLD=en:05208072545253756145; PREF=ID=529cec8133309b35:U=52920e36e39f7c30:TB=2:LD=en:NR=10:TM=1246462942:LM=1250090990:DV=AA:GM=1:IG=1:S=1Nbig7Xgwx0d0beJ; SID=DQAAAI0AAADzB_UZzHiwobkJEsG8v0KXcPNrrvrc4ejWhonrAMhPsFktUNxYyO8F9i7vXnltR9kaqS-3tmof5byfSKM4tb11iJT2pJlHvIKnY3GvXHyMOTV7VozuK6BMsqUAOUu0AdajkRmyMD2A6aavlIl1nLAMyJ8py2Uzn0CVuoGFCnv1i5oB2SkTXytoqKvgvc_JEcQ; HSID=AwuPvWlMGUJu3F7XV; SSID=AM3j2gI-CGdPsFSiN; NID=26=aiHSg4Hgdy2sZiSAir9JuVIPjzvhSG9hqRZD8V_pNWfTCmkzhCaRC0Ey3b-n4ENiPj4JBc2N8gGOKa2c92a5nNd70fzWPFipf5FX5NELk6LrM8DV3DdjeA_qddgD_SUp; rememberme=true
  //   GRLD PREF ID SID HSID SSID NID rememberme
  // Set-Cookie: SID=DQAAAIwAAAByU-HS5Gddvp1y3AIje7wZoRq1uUeaI8hH9CtmVMAYzAlBxqO0q0viJZzJ7nmHkvucxVOv-rr04jHqeO-rL_6y_RlN6HTYYKsJMQsoOuh5cYn32pmu4xBFqn95BvskA8-30-eoq9fsDdaWBEtEAFe4vcoaTdNu3ijeQaKkuDRwH3eMAHDQpmwtVmBo9o5Z2dE;Domain=.google.com;Path=/;Expires=Tue, 03-Sep-2019 11:25:23 GMT
  //   SID=;Domain=.google.com;Path=/;Expires=Tue, 03-Sep-2019 11:25:23 GMT

  // TODO: date => string
  static std::string DateFormat(int hour = 0, int day = 0, int month = 0);

private:
  typedef std::map<std::string, Item> cookie_type;
  cookie_type cookie_;
};

}
#endif // CWF_COOKIE_H__
