#ifndef BUNDLE_BUNDLE_H__
#define BUNDLE_BUNDLE_H__

#include <stdint.h>
#include <time.h>
#include <string>

namespace bundle {

class FileLock;

struct FileHeader {
  enum {
    kMAGIC_CODE = 0xE4E4E4E4,
    kVERSION = 1,
    kFILE_NORMAL = 1,
  };

  uint32_t magic_;
  uint32_t length_; // content length
  uint32_t version_;
  uint32_t flag_;
  char url_[256];
  char user_data_[512 - 256 - 4*4];
};

enum {
  kFileHeaderSize = sizeof(FileHeader),
  kUrlSize = 256,
  kUserDataSize = kFileHeaderSize - kUrlSize - 4 * sizeof(uint32_t),

  // default setting value
  kBundleHeaderSize = 1024,
  kBundleCountPerDay = 50 * 400,
  kMaxBundleSize = 2u * 1024 * 1024 * 1024u,  // 2G
};

struct Setting {
  size_t max_bundle_size;
  int bundle_count_per_day;
  int file_count_level_1;
  int file_count_level_2;
};

void SetSetting(const Setting& setting);

// const char *url, std::string *bundle_name, size_t *offset, size_t *length
typedef bool (*ExtractUrl)(const char *, std::string *, size_t *,size_t *);

// uint32_t bid, size_t offset, size_t length, const char *prefix, const char *postfix
typedef std::string (*BuildUrl)(uint32_t, size_t, size_t, const char *
  , const char *);


// TODO: uint32_t bid => string bundle_name, it's weired!

bool ExtractSimple(const char *url, std::string *bundle_name, size_t *offset
  , size_t *length);

std::string BuildSimple(uint32_t bid, size_t offset, size_t length
  , const char *prefix, const char *postfix);

std::string Bid2Filename(uint32_t bid);

/*
example:

std::string buf;
int ret = bundle::Reader::Read("p/20120424/E6/SE/Zb1/C5zWed.jpg", &buf, "/mnt/mfs");
*/
class Reader {
public:
  // string as buffer, it's not the best, but simple enough
  static int Read(const std::string &url, std::string *buf, const char *storage
    , ExtractUrl extract = &ExtractSimple, std::string *user_data = 0);

  // if OK return 0, else return error
  static int Read(const char *bundle_file, size_t offset, size_t length
    , char *buf, size_t buf_size, size_t *readed
    , const char *storage
    , char *user_data = 0, size_t user_data_size = 0);

private:
  Reader(const Reader&);
  void operator=(const Reader&);
};

/*
  char *file_buffer = "content of file";
  const int length = strlen(file_buffer);

  Writer *writer = Writer::Allocate("p/20120512", length, "/mnt/mfs");

  size_t written;
  int ret = writer->Write(file_buffer, length, &written);
  if (0 == ret)
  std::cout << "write success, url:" << writer->EnsureUrl() << std::endl;

  writer->Release(); // or delete writer
*/

class Writer {
public:
  // lock_path default is storage/.lock
  static Writer* Allocate(const char *prefix, const char *postfix
    , size_t length, const char *storage
    , const char *lock_path = 0, BuildUrl builder = &BuildSimple);

  int Write(const char *buf, size_t buf_size
    , size_t *written = 0, const char *user_data = 0, size_t user_data_size = 0) const;

  std::string EnsureUrl() const;

  int Write(const std::string &url, const char *buf, size_t buf_size
    , size_t *written, const char *user_data = 0, size_t user_data_size = 0) const;

  void Release();

  const std::string & bundle_file() const {
    return filename_;
  }
  uint32_t bundle_id() const {
    return bid_;
  }
  size_t offset() const {
    return offset_;
  }
  size_t length() const {
    return length_;
  }
  ~Writer() {
    Release();
  }

private:
  // write a file into the bundle
  static int Write(const std::string &bundle_file, size_t offset, const std::string &url
      , const char *buf, size_t buf_size
      , size_t *written
      , const char *user_data = 0, size_t user_data_size = 0);

  // forbidden create outside
  Writer()
      : filelock_(0), builder_(0), bid_(0), offset_(0), length_(0)
      {}

  // non-copy
  Writer(const Writer&);
  void operator=(const Writer&);

  std::string filename_;
  FileLock *filelock_;

  BuildUrl builder_;

  std::string prefix_, postfix_;
  uint32_t bid_;

  size_t offset_;
  size_t length_;
};

// TODO: move in .cc
template<typename T>
inline T Align1K(T v) {
  T a = v % 1024;
  return a ? (v + 1024 - a) : v;
}

}
#endif // BUNDLE_BUNDLE_H__
