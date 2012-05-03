#ifndef CWF_SITE_H__
#define CWF_SITE_H__

#include <string>

namespace cwf {

// TODO: use typed of id

class User {
public:
  std::string const& name() const { return name_; }
  int id() const { return id_; }

  User(int id = 0) : id_(id) {}
  User(const std::string& name, int id) : name_(name), id_(id) {}

  void set_name(const std::string & name) {
    name_ = name;
  }

private:
  std::string name_;
  int id_;
};

struct Request;

typedef User* (*AuthorizeType)(Request*);

}
#endif // CWF_SITE_H__
