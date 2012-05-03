#include "cwf/action.h"

#include <sstream>

#include "cwf/frame.h"

namespace cwf {

struct FooAction : public BaseAction {
  bool Match(const std::string& url) const {
    return url == "/foo";
  }

  HttpStatusCode Process(Request*, Response* res) {
    static const std::string kDefaultContentType("text/html; charset=utf-8");
    res->header().set_status_code(HC_OK, "OK");
    res->header().Add(HH_CONTENT_TYPE, kDefaultContentType);
    res->OutputHeader();
    res->WriteRaw("<h1>foo</h1>");
    return HC_OK;
  }
};


struct EatAction : BaseAction {
  bool Match(const std::string& url) const {
    return std::string::npos != url.find("eat");
  }
  HttpStatusCode Process(Request* request, Response* response) {
    static const std::string kDefaultContentType("text/html; charset=utf-8");
    response->header().set_status_code(HC_OK, "OK");
    response->header().Add(HH_CONTENT_TYPE, kDefaultContentType);
    response->OutputHeader();
    response->WriteRaw("<h1>yamm, taste good</h1>");
    std::stringstream ostem;
    ostem << "<ul>request:\n";
    for (StringMap::const_iterator i = request->query_.begin(); 
      i!=request->query_.end(); ++i) {
      ostem << "<li>" << i->first << ": " << i->second << "</li>\n";
    }
    ostem << "</ul>\n";

    ostem << "<ul>form:\n";
    for (StringMap::const_iterator i = request->form_.begin(); 
      i!=request->form_.end(); ++i) {
        ostem << "<li>" << i->first << ": " << i->second << "</li>\n";
    }
    ostem << "</ul>\n";

    ostem << "<ul>file:\n";
    for (Request::DispositionArrayType::const_iterator i = request->file_array_.begin(); 
      i!=request->file_array_.end(); ++i) {
        ostem << "<li>" << i->filename << ": " << i->data.size() << "</li>\n";
    }
    ostem << "</ul>\n";
    response->WriteRaw(ostem.str());
    return HC_OK;
  }
};

void InstallDefaultAction() {
  static EatAction eat_action_;
  static FooAction foo_action_;

  FrameWork::RegisterAction(&eat_action_);
  FrameWork::RegisterAction(&foo_action_);
}

}
