#include "bundle.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <vector>
#include <sstream>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/scoped_array.hpp>

#include "base3/mkdirs.h"
#include "base3/pathops.h"
// #include "base3/atomicops.h"

#include "bundle/murmurhash2.h"
#include "bundle/sixty.h"
#include "bundle/filelock.h"

// #define USE_CACHED_IO 1

#ifdef OS_WIN
  #include <process.h>
  #include <io.h>

  #define getpid _getpid
  #define access _access
  #define snprintf _snprintf
  #define F_OK 04

  // hack
  #define USE_CACHED_IO 0
#endif

#ifdef USE_MOOSECLIENT
  #include "mooseclient/moose_c.h"

  #define open mfs_open
  #define lstat mfs_stat
  #define read mfs_read
  #define write mfs_write
  #define close mfs_close
  #define access mfs_access
  #define unlink mfs_unlink
  #define lseek mfs_lseek
#endif

namespace bundle {

bool ExtractSimple(const char *url, Info *info) {
    std::string u(url);
    
    std::string::size_type dot = u.rfind('.');
    std::string::size_type last_slash = u.rfind('/');
    if (std::string::npos == dot || std::string::npos == dot)
      return false;
    
    if (info)
      info->prefix = u.substr(0, last_slash);

    std::string part_three = u.substr(last_slash + 1, dot - last_slash - 1);

    std::vector<std::string> vs;
    boost::split(vs, part_three, boost::is_any_of(",/._"));

    if (vs.size() < 3)
      return false;

    if (info) {
      info->id = atol(vs[0].c_str());
      info->offset = atol(vs[1].c_str());
      info->size = atol(vs[2].c_str());

      info->postfix = u.substr(dot);
    }
    return true;
}

std::string BuildSimple(const Info &info) {
  std::ostringstream ostem;

  if (!info.prefix.empty())
    ostem << info.prefix << "/";

  if (info.id != -1)
    ostem << info.id;
  else 
    ostem << info.name;

  ostem << "," 
    << info.offset << ","
    << info.size;

  if (!info.postfix.empty())
    ostem << info.postfix;
  return ostem.str();
}

bool ExtractWithEncode(const char *url, Info *info) {
  std::string u(url);
  std::string::size_type dot = u.rfind('.');
  std::string::size_type last_slash = u.rfind('/');
  if (std::string::npos == dot || std::string::npos == dot)
    return false;

  std::string coded_part = u.substr(last_slash + 1, dot - last_slash - 1);

  std::vector<std::string> vs;
  boost::split(vs, coded_part, boost::is_any_of(",/._"));

  if (vs.size() != 4)
    return false;

  uint32_t hash = FromSixty(vs[3]);
  if (hash == (uint32_t)-1)
    return false;

  std::string prefix = u.substr(0, last_slash),
    postfix = u.substr(dot);

  int id = FromSixty(vs[0]);
  size_t offset = FromSixty(vs[1]),
    size = FromSixty(vs[2]);

  {
    std::ostringstream ostem;
    ostem << prefix << "/"
      << id << "_"                       // id
      << offset << "_"
      << size << "_"
      << postfix;
    std::string str = ostem.str();
    uint32_t this_hash = MurmurHash2(str.c_str(), str.size(), 0);

    if (this_hash != hash)
      return false;
  }

  if (info) {
    info->id = id;
    info->offset = offset;
    info->size = size;
    
    info->prefix = prefix;
    info->postfix = postfix;
  }
  return true;
}

std::string BuildWithEncode(const Info &info) {
  uint32_t hash = 0;
  {
    std::ostringstream ostem;
    ostem << info.prefix << "/"
      << info.id << "_"
      << info.offset << "_"
      << info.size << "_"
      << info.postfix;
    std::string str = ostem.str();
    hash = MurmurHash2(str.c_str(), str.size(), 0);
  }

  std::ostringstream ostem;
  ostem << info.prefix << "/"
    << ToSixty(info.id) << "_" 
    << ToSixty(info.offset) << "_"
    << ToSixty(info.size) << "_"
    << ToSixty(hash)
    << info.postfix;
  return ostem.str();
}

// not thread-safe, test purpose
Setting g_default_setting = {
  kMaxBundleSize,
  kBundleCountPerDay,
  50,
  400,
  // &ExtractSimple, &BuildSimple
  &ExtractWithEncode, &BuildWithEncode
};

void SetSetting(const Setting& setting) {
  g_default_setting = setting;
}

template<typename T>
inline T Align4K(T v) {
  T a = v % 4096;
  return a ? (v + 4096 - a) : v;
}

// avoid too many files in one directory
std::string Bid2Filename(uint32_t bid) {
  char sz[18]; //8+1+8+1
#ifdef OS_WINDOWS
  snprintf(sz, sizeof(sz), "%08x\\%08x",
    bid / g_default_setting.file_count_level_1,
    bid % g_default_setting.file_count_level_2);
#else
  snprintf(sz, sizeof(sz), "%08x/%08x",
    bid / g_default_setting.file_count_level_1,
    bid % g_default_setting.file_count_level_2);
#endif
  return sz;
}

int Reader::Read(const std::string &url, std::string *buf
  , const char *storage, ExtractUrl extract, std::string *user_data) {
  if (url.empty()) {
    return -1;
  }

  if (!extract)
    extract = g_default_setting.extract;

  Info info;
  if (!extract(url.c_str(), &info))
    return -1;
  
  // 
  std::string bundle_name = base::PathJoin(storage, info.prefix);
  bundle_name = base::PathJoin(bundle_name, Bid2Filename(info.id));

  char *content = new char[info.size];
  size_t readed = 0;
  char ud[kUserDataSize] = {0};
  int ret = Read(bundle_name.c_str(), info.offset, info.size
    , content, info.size, &readed, ud, kUserDataSize);

  if (buf)
    *buf = std::string(content, readed);

  delete [] content;

  if (user_data) {
    *user_data = std::string(ud, kUserDataSize);
  }

  return ret;
}

struct AutoFile {
  AutoFile(FILE* f) : f_(f) {}
  ~AutoFile() {
    if (f_)
      fclose(f_);
  }
  FILE *f_;
};


int Reader::Read(const char *filename, size_t offset, size_t length
  , char *buf, size_t buf_size, size_t *readed
  , char *user_data, size_t user_data_size) {
  if (user_data && user_data_size < kUserDataSize)
      return EINVAL;

  FILE* fp = fopen(filename, "r+b");
  if (!fp)
    return errno;

  AutoFile af(fp);

  int ret = fseek(fp, offset, SEEK_SET);
  if (-1 == ret)
    return errno;

  FileHeader header;
  if (fread(&header, 1, kFileHeaderSize, fp) < kFileHeaderSize) {
    return EIO;
  }
  if (FileHeader::kMAGIC_CODE != header.magic_) {
    return EIO;
  }
  if (FileHeader::kVERSION != header.version_) {
    return EIO;
  }
  if (header.length_ < length) {
    return EIO;
  }
  if (FileHeader::kFILE_NORMAL != header.flag_) {
    return EIO;
  }
  int rsize = (length > buf_size)? buf_size : length;
  ret = fread(buf, 1, rsize, fp);

  if (readed)
    *readed = ret;

  if (ret < rsize) {
    return errno;
  }

  if (user_data)
    memcpy(user_data, header.user_data_, kUserDataSize);
  return 0;
}

std::string Writer::EnsureUrl() const {
  if (builder_)
    return builder_(info_);
  return "";
}

int Writer::Write(const char *buf, size_t buf_size
  , size_t *written, const char *user_data, size_t user_data_size) const {
    return Write(EnsureUrl(), buf, buf_size
      , written, user_data, user_data_size);
    return 0;
}

int Writer::Write(const std::string &url, const char *buf, size_t buf_size
  , size_t *written, const char *user_data, size_t user_data_size) const {
  return Write(filename_, info_.offset
      , url, buf, buf_size
      , written, user_data, user_data_size);
}

int Writer::Write(const std::string &bundle_file, size_t offset
  , const std::string &url
  , const char *buf, size_t buf_size
  , size_t *written
  , const char *user_data, size_t user_data_size) {
  if (written)
    *written = 0;
#if USE_CACHED_IO
  FILE *fp = fopen(bundle_file.c_str(), "r+b");
  if (!fp)
    return errno;

  AutoFile af(fp);

  int ret = fseek(fp, offset, SEEK_SET);
#else
  int fd = open(bundle_file.c_str(), O_RDWR, 0644);
  if(-1 == fd)
    return errno;
  int ret = lseek(fd, offset, SEEK_SET);
#endif

  if (-1 == ret) {
#if !USE_CACHED_IO
    close(fd);
#endif
    return EIO;
  }
  // header
  int total = Align1K(kFileHeaderSize + buf_size);
  boost::scoped_array<char> content(new char[total]);

  FileHeader *header = (FileHeader *)content.get();
  header->magic_ = FileHeader::kMAGIC_CODE;
  header->length_ = buf_size;
  header->version_ = FileHeader::kVERSION;
  header->flag_ = FileHeader::kFILE_NORMAL;
  strncpy(header->url_, url.c_str(), kUrlSize);

  if (user_data) {
    int t = std::min(user_data_size, (size_t)kUserDataSize);
    memcpy(header->user_data_, user_data, t);
  }

  memcpy(content.get() + kFileHeaderSize, buf, buf_size);

#if USE_CACHED_IO
  ret = fwrite(content.get(), sizeof(char), total, fp);
  
#else
  ret = write(fd, content.get(), total);
  close(fd);
#endif
  if (ret < 0 || ret != total) {
   return errno;
  }

  if (written)
    *written = buf_size;

  return 0;
}

int Writer::BatchWrite(const char *buf, size_t buf_size, size_t *written
  , std::string *url
  , const char *user_data, size_t user_data_size) {
  info_.size = buf_size; // for this time
  std::string this_url = EnsureUrl();
  int ret = Write(this_url, buf, buf_size, written, user_data, user_data_size);
  if (0 == ret) {
    info_.offset += Align1K(kFileHeaderSize + buf_size); // for next time

    if (url)
      *url = this_url;
  }
  return ret;
}

#if 0
time_t now = time(NULL);
struct tm* ts = localtime(&now);
snprintf(tm_buf, sizeof(tm_buf), "%04d%02d%02d",
  ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday);
#endif

bool CreateBundle(const char *filename) {
#if USE_CACHED_IO
  FILE* fp = fopen(filename, "w+b");
  if (!fp) {
    return false;
  }
#else
  int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0664);
  if (-1 == fd) {
    return false;
  }
#endif
  char now_str[100];
  {
    time_t t = time(0);
    struct tm *tmp = localtime(&t);
    strftime(now_str, 100, "%Y-%m-%d %H:%M:%S", tmp);
  }

  boost::scoped_array<char> huge(new char[kBundleHeaderSize]);
  memset(huge.get(), 0, kBundleHeaderSize);
  snprintf(huge.get(), kBundleHeaderSize, "bundle file store\n"
    "1.0\n"
    "%s\n", now_str);
#if USE_CACHED_IO
  int ret = fwrite(huge.get(), sizeof(char), kBundleHeaderSize, fp);
  fclose(fp);
  if (ret != kBundleHeaderSize) {
    return false;
  }
#else
  int ret = write(fd, huge.get(), kBundleHeaderSize);
  close(fd);
  if (ret != kBundleHeaderSize) {
    return false;
  }
#endif
  return true;
}

static int last_id_ = -1;

Writer* Writer::Allocate(const char *prefix, const char *postfix
  , size_t size, const char *storage
  , const char *lock_path, BuildUrl builder) {
  if ('/' == prefix[0])
    prefix = prefix+1;

  if (-1 == last_id_)
    last_id_ = getpid() % 10;

  std::string lock_dir;
  if (!lock_path)
    lock_dir = base::PathJoin(storage, ".lock");
  else
    lock_dir = lock_path;

  int loop_count = 0;
  while (true) {
    // if last_id_ exceed kBundleCountPerDay, random select one from another 100
    loop_count ++;
    if (loop_count > g_default_setting.bundle_count_per_day) {
      last_id_ = g_default_setting.bundle_count_per_day + rand() % 100; // TODO:
    }

    // 1 try lock
    std::string lock_file = base::PathJoin(lock_dir
      , boost::lexical_cast<std::string>(last_id_));
    FileLock *filelock = new FileLock(lock_file.c_str());
    if (!(filelock->TryLock())) {
      // check or create lock_dir
      if (-1 == access(lock_dir.c_str(), F_OK)) {
#ifndef USE_MOOSECLIENT
        if (0 != base::mkdirs(lock_dir.c_str())) {
#else
        if (0 != mfs_mkdirs(lock_dir.c_str(), 0644)) {
#endif
          delete filelock;
          return NULL;
        }
      }

      // try again
      if (!filelock->TryLock()) {
        last_id_ ++;
        delete filelock;
        continue;
      }
    }

    // 2 check bundle file (locked now)
    std::string bundle_root = base::PathJoin(storage, prefix);
    std::string bundle_file = base::PathJoin(bundle_root, Bid2Filename(last_id_));
    struct stat stat_buf;
    int stat_ret;
    if ((0 == (stat_ret = lstat(bundle_file.c_str(), &stat_buf))) &&
        (stat_buf.st_size + Align1K(kFileHeaderSize + size))
          > g_default_setting.max_bundle_size) {
      last_id_ ++;
      delete filelock;
      continue;
    }
    int stat_errno = errno;

    // 3 maybe create bundle
    size_t offset;
    if ((-1 == stat_ret) && (stat_errno == ENOENT)) {
      // get parent dir
      std::string parent = base::Dirname(bundle_file);
      // store dir check
      if (-1 == access(parent.c_str(), F_OK)) {
#ifndef USE_MOOSECLIENT
        if (0 != base::mkdirs(parent.c_str())) {
#else
        if (0 != mfs_mkdirs(parent.c_str(), 0644)) {
#endif
          delete filelock;
          return NULL;
        }
      }

      if (!CreateBundle(bundle_file.c_str())) {
        delete filelock;
        return NULL;
      }
      offset = kBundleHeaderSize;
    }
    else {
      // bundle file OK
      if (0 == stat_ret) {
        offset = stat_buf.st_size;
      }
      else{
        last_id_ ++;
        continue;
      }
    }

    // 4
    Writer * w = new Writer();
    w->filename_ = bundle_file;
    w->filelock_ = filelock;
    if (builder)
      w->builder_ = builder; // TODO:
    else
      w->builder_ = g_default_setting.build;

    w->info_.id = last_id_;
    w->info_.offset = offset;
    w->info_.size = size;

    w->info_.prefix = prefix;
    w->info_.postfix = postfix;
    
    return w;
  } // while(true)

  return NULL;
}

void Writer::Release() {
  delete filelock_;
  filelock_ = NULL;
}

} // namespace bundle
