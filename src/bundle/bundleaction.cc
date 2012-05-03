#include <stdio.h> // snprintf

#include "base3/startuplist.h"
#include "base3/globalinit.h"
#include "base3/logging.h"

#include "cwf/action.h"
#include "cwf/frame.h"
#include "bundle/bundle.h"

using namespace cwf;

namespace bundle {

const char *the_storage_ = "test";

struct WriteAction : public BaseAction {
  virtual bool Match(const std::string& url) const {
    return url.find("bundle") != std::string::npos;
  }

  virtual HttpStatusCode Process(Request * request, Response * response) {
    static const std::string kDefaultContentType("text/plain; charset=utf-8");

    response->header().set_status_code(HC_OK, "OK");
    response->header().Add(HH_CONTENT_TYPE, kDefaultContentType);
    response->OutputHeader();

    const Request::DispositionArrayType & files = request->files();
    for (unsigned int i = 0; i < files.size(); ++i) {
      const cwf::Request::FormDisposition & fd = files[i];

      // prefix like: "p/20120512"
      char tm_buf[32];
      time_t now = time(NULL);
      struct tm* ts = localtime(&now);
      snprintf(tm_buf, sizeof(tm_buf), "p/%04d%02d%02d",
        ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday);

      // TODO: allocate once, use multi-times
      Writer* aw = Writer::Allocate(tm_buf, ".jpg", fd.data.size(), the_storage_);
      
      int ret = aw->Write(fd.data.c_str(), fd.data.size());

      if (0 == ret) {
        std::string url = aw->EnsureUrl();
        url += '\n';
        response->WriteRaw(url);
      }
      else {
      	LOG(INFO) << "Write failed, err " << ret;
      	std::ostringstream ostem;
      	ostem << "failed " << ret;
      	response->WriteRaw(ostem.str());
      }

      delete aw;
    }
    return HC_OK;
  }
};

struct ReadAction : public cwf::BaseAction {
  virtual bool Match(const std::string& url) const {
    return url.find("/p/") == 0;
  }

  virtual cwf::HttpStatusCode Process(cwf::Request * request, cwf::Response * response) {
    static const std::string kImageType("image/jpeg");

    std::string url = request->url();
#if 0
    if (url.find("/p/") != 0) {
      response->header().set_status_code(cwf::HC_NOT_ACCEPTABLE, "Not Found");
      response->OutputHeader();
      return cwf::HC_NOT_ACCEPTABLE;
    }
#endif

    // ugly 3
    url = url.substr(1);
    bool f = ExtractSimple(url.c_str(), 0, 0, 0);
    if (!f) {
      response->header().set_status_code(cwf::HC_NOT_FOUND, "Not Found");
      response->OutputHeader();
      return cwf::HC_NOT_FOUND;
    }

    std::string buf;

    int ret = Reader::Read(url, &buf, the_storage_);
    if (ret == 0) {
      response->header().set_status_code(cwf::HC_OK, "OK");
      response->header().Add(cwf::HH_CONTENT_TYPE, kImageType);
      char sz[16];
      snprintf(sz, 16, "%d", buf.size());
      response->header().Add(cwf::HH_CONTENT_LENGTH, sz);
      response->WriteRawWithHeader(buf.c_str(), buf.size());
    } else {
      response->header().set_status_code(cwf::HC_INTERNAL_SERVER_ERROR, "Server Error");
      response->OutputHeader();
      return cwf::HC_INTERNAL_SERVER_ERROR;
    }
    return cwf::HC_OK;
  }
};

static void Init() {
  FrameWork::RegisterAction(new WriteAction);
  FrameWork::RegisterAction(new ReadAction);
}

}

GLOBAL_INIT(BUNDLEACTION, {
  base::AddStartup(&bundle::Init);
});
