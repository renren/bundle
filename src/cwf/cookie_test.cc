#include <gtest/gtest.h>
#include <iostream>
#include "cwf/cookie.h"


using namespace cwf;

TEST(a,parse) {
  const char* arr[] = {
    "Cookie: GRLD=en:05208072545253756145; PREF=ID=529cec8133309b35:U=52920e36e39f7c30:TB=2:LD=en:NR=10:TM=1246462942:LM=1250090990:DV=AA:GM=1:IG=1:S=1Nbig7Xgwx0d0beJ; SID=DQAAAI0AAADzB_UZzHiwobkJEsG8v0KXcPNrrvrc4ejWhonrAMhPsFktUNxYyO8F9i7vXnltR9kaqS-3tmof5byfSKM4tb11iJT2pJlHvIKnY3GvXHyMOTV7VozuK6BMsqUAOUu0AdajkRmyMD2A6aavlIl1nLAMyJ8py2Uzn0CVuoGFCnv1i5oB2SkTXytoqKvgvc_JEcQ; HSID=AwuPvWlMGUJu3F7XV; SSID=AM3j2gI-CGdPsFSiN; NID=26=aiHSg4Hgdy2sZiSAir9JuVIPjzvhSG9hqRZD8V_pNWfTCmkzhCaRC0Ey3b-n4ENiPj4JBc2N8gGOKa2c92a5nNd70fzWPFipf5FX5NELk6LrM8DV3DdjeA_qddgD_SUp; rememberme=true",
  };

  Cookie c;
  EXPECT_TRUE(c.Parse(arr[0]));
  EXPECT_TRUE(c.Get("GRLD") == "en:05208072545253756145");
  EXPECT_TRUE(c.Get("PREF") == "ID=529cec8133309b35:U=52920e36e39f7c30:TB=2:LD=en:NR=10:TM=1246462942:LM=1250090990:DV=AA:GM=1:IG=1:S=1Nbig7Xgwx0d0beJ");
}

TEST(a,iterface) {
  Cookie c;
  Cookie::Item& i = c.Add("GRLD");
  i.domain = "xx.com";
  i.path = "/data";
}

TEST(a,text) {
  std::cout << Cookie::DateFormat(1) << std::endl;
}

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc,argv); 
  int r = RUN_ALL_TESTS();
  return r;
}
