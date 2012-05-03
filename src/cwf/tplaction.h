#ifndef CELL_CWF_TPL_ACTION_H__
#define CELL_CWF_TPL_ACTION_H__

#include "ctemplate/template.h"
#include "ctemplate/template_dictionary.h"

#include "cwf/action.h"
#include "cwf/stream.h"

namespace cwf {

struct TemplateAction : public BaseAction {
  bool Match(const std::string& url) const {
    return url == "/too";
  }

  cwf::HttpStatusCode Process(Request*, Response* res) {
    static const std::string kDefaultContentType("text/html; charset=utf-8");
    res->header().set_status_code(cwf::HC_OK, "OK");
    res->header().Add(cwf::HH_CONTENT_TYPE, kDefaultContentType);
    res->OutputHeader();

    ctemplate::TemplateDictionary dict("too");
    dict.SetValue("URL", "too");

    ctemplate::Template * tpl = ctemplate::Template::GetTemplate(
      "404.tpl", ctemplate::STRIP_WHITESPACE);
    if (tpl) {
      std::string out;
      tpl->Expand(&out, &dict);
      res->WriteRaw(out);
    }

    return cwf::HC_OK;
  }
};

}
#endif // CELL_CWF_TPL_ACTION_H__
