#ifndef CWF_ACTION_H__
#define CWF_ACTION_H__

#include <string>
#include "cwf/http.h"
#include "cwf/stream.h"
#include "cwf/site.h"

namespace cwf {

struct BaseAction {
  virtual bool Match(const std::string& url) const = 0;
  virtual HttpStatusCode Process(Request*, Response*) = 0;

#if 0
  virtual User* Authorize(Request*) = 0;
  virtual const std::string& template_name() const = 0;
  virtual HttpStatusCode GenerateHeader(Request* request, Header* header) = 0;
  virtual ctemplate::TemplateDictionary* Process(Request* request) = 0;
#endif
  virtual ~BaseAction() {}
};

}
#endif // CWF_ACTION_H__
