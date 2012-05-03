#include "cwf/tplaction.h"

#include "base3/logging.h"
#include "base3/startuplist.h"
#include "base3/globalinit.h"
#include "base3/signals.h"

static void SignalReload(int) {
  LOG(INFO) << "reload template";
  ctemplate::Template::ReloadAllIfChanged();
}

// 静态函数可能添加不上，需要main内部再显示添加一次
static void Init() {
  base::InstallSignal(BASE_SIGNAL_CWF_LOAD_TEMPLATE, &SignalReload);

  ctemplate::Template::SetTemplateRootDirectory("/data/xce/XFeed/etc/feed_view_tpl");
  ctemplate::Template::AddAlternateTemplateRootDirectory("/data/cwf/tpl/");
  ctemplate::Template::AddAlternateTemplateRootDirectory(".");
}

GLOBAL_INIT(TPLACTION, {
  base::AddStartup(&Init);
});
