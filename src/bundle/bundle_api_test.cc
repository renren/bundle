#include "gtest/gtest.h"

#include "bundle/bundle.h"

TEST(Bundle, Example) {
  using namespace bundle;

  const char *file_buffer = "content of file";
  const int file_size = strlen(file_buffer);

  Writer *writer = Writer::Allocate("p/20120512", ".jpg", file_size, ".");

  std::string url = writer->EnsureUrl();

  std::cout << "url: " << writer->EnsureUrl() << std::endl;
  // p/20120512/1_2_3_4.jpg

  size_t written;
  int ret = writer->Write(file_buffer, file_size, &written);
  std::cout << "return: " << ret 
    << " written: " << written << std::endl;

  writer->Release(); // or delete writer

  //
  {
    std::string buf;
    int ret = bundle::Reader::Read(url, &buf, ".");
    EXPECT_EQ(buf, file_buffer);
  }
}
