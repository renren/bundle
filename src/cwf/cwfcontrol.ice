#ifndef XCE_CWF_CONTROL_ICE__
#define XCE_CWF_CONTROL_ICE__

module cwf {

sequence<string> StringList;

interface Control {
  // jdbcurl 
  // jdbc:mysql://[host][,failoverhost...][:port]/[database]
  // jdbc:mysql://[host][,failoverhost...][:port]/[database][?propertyName1][=propertyValue1][&propertyName2][=propertyValue2]...

  // reply_template.id
  void ReloadTemplate(string jdbcurl, StringList ids);
};

};
#endif // XCE_CWF_CONTROL_ICE__
