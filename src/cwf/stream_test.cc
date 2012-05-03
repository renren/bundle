#include <gtest/gtest.h>
#include "cwf/stream.h"

const char * header_1 = "POST /f/big HTTP/1.1\r\n"
"Host: 10.2.18.202\r\n"
"User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9.2.13) Gecko/20101206 Ubuntu/10.10 (maverick) Firefox/3.6.13 GTB7.1\r\n"
"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
"Accept-Language: en-us,en;q=0.5\r\n"
"Accept-Encoding: gzip,deflate\r\n"
"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n"
"Keep-Alive: 115\r\n"
"Connection: keep-alive\r\n"
"Referer: http://10.2.18.202/form.html\r\n"
"Content-Type: multipart/form-data; boundary=---------------------------156828763110148648651961632474\r\n"
"Content-Length: 481\r\n\r\n";

const char * content_1 = 
"-----------------------------138505317616198927051125376428\r\n"
"Content-Disposition: form-data; name=\"name_of_sender\"\r\n"
"\r\n"
"hello\r\n"
"-----------------------------138505317616198927051125376428\r\n"
"Content-Disposition: form-data; name=\"keep_empty\"\r\n"
"\r\n"
"\r\n"
"-----------------------------138505317616198927051125376428\r\n"
"Content-Disposition: form-data; name=\"t\"\r\n"
"\r\n"
"dd\r\n"
"\r\n"
"ee\r\n"
"ff\r\n"
"\r\n"
"\r\n"
"-----------------------------138505317616198927051125376428\r\n"
"Content-Disposition: form-data; name=\"name_of_files\"; filename=\"foo\"\r\n"
"Content-Type: application/octet-stream\r\n"
"\r\n"
"Error: hello\r\n"
"\r\n"
"ccccccccccccccccccccccc\r\n"
"\r\n"
"ddddddddddddddddddd\r\n"
"\r\n"
"-----------------------------138505317616198927051125376428--\r\n"
;

TEST(Form, Parse) {
  cwf::Request::DispositionArrayType a = cwf::Request::ParseMultipartForm("multipart/form-data; boundary=---------------------------138505317616198927051125376428"
    , content_1);

  EXPECT_EQ("name_of_sender", a[0].name);
  EXPECT_EQ("hello", a[0].data);
  EXPECT_TRUE(a[0].filename.empty());

  EXPECT_EQ("keep_empty", a[1].name);
  EXPECT_EQ("", a[1].data);
  EXPECT_TRUE(a[1].filename.empty());

  EXPECT_EQ("t", a[2].name);
  EXPECT_EQ("dd\r\n\r\nee\r\nff\r\n\r\n", a[2].data);
  EXPECT_TRUE(a[2].filename.empty());

  EXPECT_EQ("name_of_files", a[3].name);
  const char * p = "Error: hello\r\n"
    "\r\n"
    "ccccccccccccccccccccccc\r\n"
    "\r\n"
    "ddddddddddddddddddd\r\n";
  EXPECT_STREQ(p, a[3].data.c_str());
  EXPECT_EQ("foo", a[3].filename);
  EXPECT_EQ("application/octet-stream", a[3].content_type);
}