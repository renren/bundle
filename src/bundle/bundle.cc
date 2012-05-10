#include "bundle.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
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

#define USE_CACHED_IO 1

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

  int32_t hash = FromSixty(vs[3]);
  if (hash == -1)
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


/*
// bundle_name = prefix + "/" + bid_str
// bundle_file = storage + bundle_name
bool ExtractSimple(const char *url, std::string *bundle_name
  , size_t *offset, size_t *length) {
  if (!url) {
    return false;
  }
  char* arr[6];
  boost::scoped_array<char> url_buf(new char[strlen(url) + 1]);
  strcpy(url_buf.get(), url);
  char* p1;
  char delim1 = '/';
  int i;
  //1
  for (i = 4 ; i > 0; i--) {
    if ((p1 = strrchr(url_buf.get(), delim1))) {
      arr[i] = p1+1;
      *p1 = '\0';
    }
    else
      break;
  }
  if (i > 0) {
    return false;
  }
  arr[0] = url_buf.get();
  p1 = strchr(arr[4], '.');
  arr[5] = p1 + 1;
  *p1 = '\0';

  //arr[4]= sixth(hash)
  //arr[3]= sixty(length);arr[2]=sixty(offset); arr[1]=sixty(bid); arr[0]=prefix
  uint32_t hash;
  size_t length_s, offset_s, bid_s;
  if (-1 == (hash = FromSixty(arr[4])))
    return false;
  if (-1 == (length_s = FromSixty(arr[3])))
    return false;
  if (-1 == (offset_s = FromSixty(arr[2])))
    return false;
  if (-1 == (bid_s = FromSixty(arr[1])))
    return false;

  //check
  std::ostringstream ostem;
  ostem << arr[0] << "/"
    << std::ios::hex << bid_s << "/"
    << std::ios::hex << offset_s << "/"
    << std::ios::hex << length_s << "."
    << arr[5];
  std::string s = ostem.str();
  if (hash == MurmurHash2(s.c_str(), s.size(), 0)) {
    if (offset)  *offset = offset_s;
    if (length)  *length = length_s;
    if (bundle_name)
      *bundle_name =base::PathJoin(arr[0], Bid2Filename(bid_s));
    return true;
  }
  else {
    return false;
  }
}

// hash = sixty(prefix/bid/offset/lengthpostfix)
// prefix/sixty(bid)/sixty(offset)/sixty(length)/sixty(hash).postfix
// fmn04/large/20110919/ K/BkUlRx3O/Dp0WdL.jpg
// fmn05/xxx/x/20110930/ Gq/FH1PM2bygR/BzdJXz.jpg
std::string BuildSimple(uint32_t bid, size_t offset, size_t length
  , const char *prefix, const char *postfix) {
  uint32_t a = 0;
  std::ostringstream ostem;
  ostem << prefix << "/"
    << std::ios::hex << bid << "/"
    << std::ios::hex << offset << "/"
    << std::ios::hex << length
    << postfix;
  std::string s = ostem.str();
  a = MurmurHash2(s.c_str(), s.size(), 0);

  //a |= length << 32;
  if (!(ToSixty(bid).size() > 0))
    return std::string();
  std::ostringstream ostem_url;
  ostem_url << prefix << "/"
    << ToSixty(bid) << "/"
    << ToSixty(offset) << "/"
    << ToSixty(length) << "/"
    << ToSixty(a) << postfix;
  return ostem_url.str();
}
*/

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
  if (extract(url.c_str(), &info)) {
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
    return ENOENT;

  AutoFile af(fp);

  int ret = fseek(fp, offset, SEEK_SET);
#else
  int fd = open(bundle_file.c_str(), O_RDWR);
  if(-1 == fd)
    return ENOENT;
  int ret = lseek(fd, offset, SEEK_SET);
#endif

  if (-1 == ret) {
    // TODO: close(fd);
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

#if 0
time_t now = time(NULL);
struct tm* ts = localtime(&now);
snprintf(tm_buf, sizeof(tm_buf), "%04d%02d%02d",
  ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday);
#endif

static int last_id_ = -1;

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

Writer* Writer::Allocate(const char *prefix, const char *postfix
  , size_t size, const char *storage
  , const char *lock_path, BuildUrl builder) {
  if ('/' == prefix[0])
    prefix = prefix+1;

  // TODO: lock
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

    // 1 check file
    std::string bundle_root = base::PathJoin(storage, prefix);
    std::string bundle_file = base::PathJoin(bundle_root, Bid2Filename(last_id_));
    struct stat stat_buf;
    int stat_ret;
    if ((0 == (stat_ret = stat(bundle_file.c_str(), &stat_buf))) &&
        (stat_buf.st_size + Align1K(kFileHeaderSize + size))
          > g_default_setting.max_bundle_size) {
      last_id_ ++;
      continue;
    }
    int stat_errno = errno;

    // 2 try lock
    // lock path check
    if (-1 == access(lock_dir.c_str(), F_OK)) {
      if(0 != base::mkdirs(lock_dir.c_str())) {
        return NULL;
      }
    }

    //
    std::string lock_file = base::PathJoin(lock_dir
        , boost::lexical_cast<std::string>(last_id_));
    FileLock *filelock = new FileLock(lock_file.c_str());
    if (!(filelock->TryLock())) {
      last_id_ ++;
      delete filelock;
      continue;
    }

    // 3 locked
    size_t offset;
    if ((-1 == stat_ret) && (stat_errno == ENOENT)) {
      // get parent dir
      std::string parent = base::Dirname(bundle_file);
      // store dir check
      if (-1 == access(parent.c_str(), F_OK)) {
        if (0 != base::mkdirs(parent.c_str())) {
          delete filelock;
          return NULL;
        }
      }

      if (!CreateBundle(bundle_file.c_str())) {
        // TODO: use logging
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
  if (filelock_) {
    delete filelock_;
    filelock_ = NULL;
  }
}

} // namespace bundle
