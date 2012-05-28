#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include <sstream>
#include <iostream>

#include "bundle/bundle.h"

std::string Write(const char *prefix, const char *postfix, const char *storage
  , long *tallocate, long *twrite) {
  int loop =1;  
  char ss[1024];
  std::ostringstream fcontent;
  while(!feof(stdin)) {
    int ret = fread(ss, 1, sizeof(ss), stdin);
    fcontent.write(ss, ret);
  }
  
  int file_size = fcontent.str().length();

  struct timeval t_start, t_enda, t_endw;
  gettimeofday(&t_start, NULL);
  bundle::Writer* writer = bundle::Writer::Allocate(prefix, postfix
    , file_size, storage);
  gettimeofday(&t_enda, NULL);
  *tallocate = (t_enda.tv_sec*1000 + t_enda.tv_usec/1000)
    - (t_start.tv_sec*1000 + t_start.tv_usec/1000);

  if (writer) {
    size_t written;
    int ret = writer->Write((fcontent.str()).c_str(), file_size, &written);
    gettimeofday(&t_endw, NULL);
    *twrite = (t_endw.tv_sec*1000 + t_endw.tv_usec/1000)
      - (t_enda.tv_sec*1000 + t_enda.tv_usec/1000);
    std::string url = writer->EnsureUrl();
    delete writer;
    return url;
  }
  return "";
}

#if 1
int main(int argc, char *argv[]) {
  char tm_buf[32];
  time_t now = time(NULL);
  struct tm* ts = localtime(&now);
  snprintf(tm_buf, sizeof(tm_buf), "b/%04d%02d%02d",
    ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday);

  const char *storage = ""; // cwd
  if (argc > 1)
    storage = argv[1];

  struct timeval t_start, t_end;
  gettimeofday(&t_start, NULL);
  long dur_a, dur_w;
  std::string surl = Write(tm_buf, ".jpg", storage, &dur_a, &dur_w);
  gettimeofday(&t_end, NULL);
  long duration = (t_end.tv_sec*1000 + t_end.tv_usec/1000) 
    - (t_start.tv_sec*1000 + t_start.tv_usec/1000);

  if (argc > 2) {
    std::cout << surl << " " << argv[2] << " " << duration
      << " "<< dur_a << " " << dur_w << std::endl;
  }
  else
    std::cout << surl << " " << duration << " " << dur_a << " " << dur_w 
      << std::endl;
  return 0;
}
#endif
