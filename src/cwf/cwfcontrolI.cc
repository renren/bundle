#include "base3/logging.h"
#include "base3/startuplist.h"
#include "arch/service.h"

#include "cwf/connect.h"
// #include "cwf/replyaction.h"
#include "cwf/cwfcontrol.h"

namespace xce {
/*

desc reply_data_schemas;

+----------+------------------+------+-----+---------+----------------+
| Field    | Type             | Null | Key | Default | Extra          |
+----------+------------------+------+-----+---------+----------------+
| id       | int(11) unsigned | NO   | PRI | NULL    | auto_increment | 
| key_list | varchar(1024)    | NO   |     |         |                |
| name     | varchar(128)     | NO   |     |         |                | 
+----------+------------------+------+-----+---------+----------------+

*/

static void SoInit() {
  // 1 init ctemplate
  // get jdbcurl
  // ReloadTemplate(url, 0);

  // 2 create fastcgi connection
  cwf::FastcgiConnect("0:0:0:0", 3100);
}

class CWFControl : public cwf::Control {
public:
  CWFControl() {
    AddStartup(&SoInit);
  }

  virtual void ReloadTemplate(const ::std::string& jdbcurl, const ::cwf::StringList& ids
    , const ::Ice::Current& = ::Ice::Current()) {
    LOG(INFO) << "ReloadTemplate " << jdbcurl;
    ReloadTemplate(jdbcurl, ids);
  }
};

}

BEGIN_SERVANT_MAP("cwf")
  SERVANT_ENTRY(reply, &xce::ServantFactory<xce::CWFControl>::CreateServant
    , &xce::ServantFactory<xce::CWFControl>::Destroy)
END_SERVANT_MAP()
