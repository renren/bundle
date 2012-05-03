#ifndef CWF_TASK_H__
#define CWF_TASK_H__

#include <string>
#include "ctemplate/template_dictionary.h"
#include "cwf/http.h"
#include "cwf/stream.h"
#include "cwf/site.h"

namespace cwf {

struct Task {
  virtual bool GenerateHeader(Request* request, Header*) = 0;

  virtual Task* MakeCopy() = 0;
  virtual bool Start(Request* request) = 0;
  virtual void Cancel() = 0;

  virtual bool Build(ctemplate::TemplateDictionary*) = 0;
  // virtual bool BuildSection(const char* section_name, ctemplate::TemplateDictionary* parent_dict) = 0;
  // virtual bool BuildInclude(ctemplate::TemplateDictionary*) = 0;
};

}
#endif // CWF_TASK_H__
