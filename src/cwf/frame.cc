#include "cwf/frame.h"

#if defined(POSIX) || defined(OS_LINUX)
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#endif

#include <boost/foreach.hpp>
#include <boost/thread.hpp>

#include "3rdparty/libfcgi/fcgiapp.h"

#include "base3/common.h"
#include "base3/logging.h"
#include "base3/startuplist.h"
#include "base3/ptime.h"
#include "base3/signals.h"
#include "base3/metrics/stats_counters.h"
#include "base3/logging.h"

#include "cwf/action.h"
#include "cwf/pattern.h"

namespace xce {
extern cwf::User* Authorize(cwf::Request* q);
}

namespace cwf {

static const std::string kDefaultContentType("text/html; charset=utf-8");

void GenerateCommonHeader(Header* header, HttpStatusCode status_code
        , std::string const & message, std::string const& content_type = "") {
  // HTTP/1.x 200 OK
  header->set_status_code(status_code, message);
  if (!content_type.empty())
    header->Add(HH_CONTENT_TYPE, content_type);
}

bool FrameWork::LoadConfig(const std::string& filename) {
  // TODO: 可以实现一个强大的配置体系，支持 so 动态加载
  return true;
}

bool FrameWork::InitHost(bool load_action) {
  // 把 default_action_ 复制到 host_action_
  if (default_actions_)
    host_action_.assign(default_actions_->begin(), default_actions_->end());
  return true;
}

BaseAction* FrameWork::Find(std::string const& url) const {
  for (ActionListType::const_iterator i=host_action_.begin();
    i!= host_action_.end(); ++i) {
      BaseAction* ha = *i;
      if (ha->Match(url))
        return ha;
  }
  return 0;
}

// XAR_IMPL(cwferr);
// XAR_IMPL(cwfall);
// XAR_IMPL(prcGT100);
HttpStatusCode FrameWork::Process(Request* request, Response* response) {
  BaseAction* a = Find(request->url());
  if (!a) {
    ResponseError(response, HC_NOT_FOUND);
    return HC_NOT_FOUND;
  }

  // TODO: access log here
  HttpStatusCode rc = a->Process(request, response);

  if (HC_OK != rc)
    ResponseError(response, rc);
  
  return rc;
}

FrameWork::ActionListType * FrameWork::default_actions_ = 0;

void FrameWork::RegisterAction(BaseAction* a) {
  if (!default_actions_) {
    default_actions_ = new ActionListType();
  }
  default_actions_->push_back(a);
}

void FrameWork::ListAction(std::ostream & ostem) {
  // 应该先调用 RunStartupList
  if (!default_actions_)
    return;

  for (FrameWork::ActionListType::const_iterator i = default_actions_->begin();
    i != default_actions_->end(); ++i) {
      ostem << typeid(**i).name() << std::endl;
  }
}

struct CodeWithMessage {
  HttpStatusCode code;
  const char* message;
};

const CodeWithMessage kDefaultMessage[] = {
  {HC_BAD_REQUEST, "Bad Request"},
  {HC_UNAUTHORIZED, "Unauthorized"},

  {HC_OK, 0}
};

void FrameWork::ResponseError(Response* response, HttpStatusCode code, const char* message) {
  if (!message) {
    for (const CodeWithMessage* cm = &kDefaultMessage[0]; cm->message; cm ++)
      if (cm->code == code) {
        message = cm->message;
        break;
      }

    if (!message)
      message = "TODO:";
  }
  response->header().set_status_code(code, message);
  response->OutputHeader();
}

static bool quit_ = false;
static int sock_ = 0;

void SignalQuit(int) {
  quit_ = true;

#if defined(OS_LINUX)
  // exit(0);
  // TODO: not work below
  // int f = socket(PF_INET, SOCK_STREAM, 0);
  // connect(sock_, 0, 0);
  // close(f);
#endif
}

void FastcgiProc(FrameWork* fw, int fd) {
  FCGX_Request wrap;
  int ret = FCGX_InitRequest(&wrap, fd, 0);
  LOG_ASSERT(0 == ret) << "FCGX_InitRequest failed";

  base::StatsCounter request_count("RequestCount");

  while (FCGX_Accept_r(&wrap) >= 0) {
    base::ptime pt("", false);

    Request* q = new Request();
    if (!q->Init(wrap.in, wrap.envp)) {
      LOG(ERROR) << "Request::Init failed.";
    }

    Response* p = new Response(wrap.out, wrap.err);

    HttpStatusCode rc = HC_SERVICE_UNAVAILABLE;
    try {
      rc = fw->Process(q, p);
    } catch(...) {
      LOG(ERROR) << "Process failed"; // TODO: log stack trace
    }

    LOG(INFO) << pt.wall_clock() << " " << rc << " " << q->url();

    request_count.Increment();

    delete p;
    delete q;

    if (quit_)
      break;
  }

  FCGX_Finish_r(&wrap);
}

extern void InstallDefaultAction();

int FastcgiMain(int thread_count, int fd, const char * log_filename) {
#if 0
  // TODO: remove, not need anymore
  if (log_filename) {
    using namespace logging;
    InitLogging(log_filename, LOG_ONLY_TO_FILE
      , DONT_LOCK_LOG_FILE
      , APPEND_TO_OLD_LOG_FILE);

    // pid, thread_id, timestamp, tickcount
    SetLogItems(true, true, true, false);
  }
#endif

  base::RunStartupList();

#if defined(OS_LINUX)
  base::InstallSignal(SIGINT, SignalQuit);
  base::InstallSignal(SIGTERM, SignalQuit);
#endif

  std::auto_ptr<FrameWork> fw(new FrameWork());
  if (!fw->LoadConfig("cwf.conf")) {
    LOG(INFO) << "config load failed";
    LOG(ERROR) << "TODO: config error\n";
    return 1;
  }

  if (!fw->InitHost(true)) {
    LOG(ERROR) << "TODO: InitHost error reason\n";
    return 1;
  }

  int ret = FCGX_Init();
  LOG_ASSERT(ret == 0) << "FCGX_Init result " << ret;

  sock_ = fd;

  boost::thread_group g;
  for (int i=1; i<thread_count; ++i)
    g.add_thread(new boost::thread(
      boost::bind(&FastcgiProc, fw.get(), fd)
    ));

  FastcgiProc(fw.get(), fd); // TODO: 管理 ....
  LOG(INFO) << "FastcgiMain quit";
  return 0;
}

}
