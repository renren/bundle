#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dlfcn.h>

#include <errno.h>
#include <cassert>
#include <string>
#include "base3/hashmap.h"

#include <boost/algorithm/string/predicate.hpp>

#include "base3/pathops.h"
#include "mooseclient/moose.h"

extern "C" {
int open(const char *pathname, int flags);
int creat(const char *pathname, mode_t mode);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
ssize_t pread(int fd, void *buf, size_t count, off_t offset);
ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);
int close(int fd);
off_t lseek(int fd, off_t offset, int whence);

static int (*real_open)(const char *pathname, int flags) = NULL;
static int (*real_creat)(const char *pathname, mode_t mode) = NULL;
static ssize_t (*real_read)(int fd, void *buf, size_t count) = NULL;
static ssize_t (*real_write)(int fd, const void *buf, size_t count) = NULL;
static ssize_t (*real_pread)(int fd, void *buf, size_t count, off_t offset) = NULL;
static ssize_t (*real_pwrite)(int fd, const void *buf, size_t count, off_t offset) = NULL;
static int (*real_close)(int fd) = NULL;
static off_t (*real_lseek)(int fd, off_t offset, int whence) = NULL;
};

#define LOAD_FUNC(name)                                                 \
        do {                                                            \
          *(void**) (&real_##name) = dlsym(RTLD_NEXT, #name);     \
          assert(real_##name);                                    \
        } while (false)

#define LOAD_FUNC_VERSIONED(name, version)                              \
        do {                                                            \
          *(void**) (&real_##name) = dlvsym(RTLD_NEXT, #name, version); \
          assert(real_##name);                                    \
        } while (false)

static void load_functions(void) {
  static volatile bool loaded = false;

  if (loaded)
    return;

  LOAD_FUNC(open);
  LOAD_FUNC(creat);
  LOAD_FUNC(read);
  LOAD_FUNC(write);
  LOAD_FUNC(pread);
  LOAD_FUNC(pwrite);
  LOAD_FUNC(close);
  LOAD_FUNC(lseek);

  loaded = true;
}

static std::string mount_point_;
typedef std::hash_map<int, moose::File*> TrapedMapType;
static TrapedMapType map_;
static moose::MasterServer *master_ = NULL;

static moose::File* GetFile(int fd) {
  TrapedMapType::iterator i = map_.find(fd);
  if (i == map_.end())
    return NULL;
  return i->second;
}

std::string ToAbspath(const char *pathname) {
  if (!pathname)
    return "";

  std::string b(pathname);
  if (base::IsAbspath(b))
    return b;

  char cwd[260];
  getcwd(cwd, sizeof(cwd));
  return base::PathJoin(cwd, b);
}

std::string ToInterpath(std::string const &abspath) {
  // left strip mount_point_
  return abspath.substr(mount_point_.size());
}

bool IsTrappedFile(const std::string &pathname) {
  std::string a(pathname);

  if (a[a.size() - 1] != '/')
    a += '/';

  size_t pos = a.find(mount_point_);
  if (pos != 0) 
    return false;

  if (a.size() == pos)
    return true;

  if (a.size() > pos && a[pos] == '/')
    return true;
  return false;
}

static bool ConnectReady() {
  return !mount_point_.empty() && master_ && master_->session_id();
}

int open(const char *pathname, int flags) {
  std::string abspath = ToAbspath(pathname);
  if (IsTrappedFile(abspath) && ConnectReady()) {
    std::string interpath = ToInterpath(abspath);
    moose::File * f = new moose::File(master_);
    if (0 == f->Open(pathname, flags, 0)) {
      map_[f->inode()] = f;
      return f->inode();
    }

    delete f;
    return -1;
  }
  load_functions();
  return real_open(pathname, flags);
}

int creat(const char *pathname, mode_t mode) {
  std::string abspath = ToAbspath(pathname);
  if (IsTrappedFile(abspath) && ConnectReady()) {
    std::string interpath = ToInterpath(abspath);
    moose::File * f = new moose::File(master_);
    if (0 == f->Create(pathname, mode)) {
      int fd = f->inode();
      map_[fd] = f;
      return fd;
    }

    delete f;
    return -1;
  }
  load_functions();
  return real_creat(pathname, mode);
}

ssize_t read(int fd, void *buf, size_t count) {
  moose::File *f = GetFile(fd);
  if (f && ConnectReady()) {
    return f->Read((char *)buf, count);
  }
  load_functions();
  return real_read(fd, buf, count);
}

ssize_t write(int fd, const void *buf, size_t count) {
  // real_write(2, "in fake write\n", 14);
  moose::File *f = GetFile(fd);
  if (f && ConnectReady()) {
    return f->Write((const char *)buf, count);
  }
  load_functions();
  return real_write(fd, buf, count);
}

ssize_t pread(int fd, void *buf, size_t count, off_t offset) {
  moose::File *f = GetFile(fd);
  if (f && ConnectReady()) {
    return f->Pread((char *)buf, count, offset);
  }
  load_functions();
  return real_pread(fd, buf, count, offset);
}

ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset) {
  moose::File *f = GetFile(fd);
  if (f && ConnectReady()) {
    return f->Pwrite((const char *)buf, count, offset);
  }
  load_functions();
  return real_pwrite(fd, buf, count, offset);
}

int close(int fd) {
  moose::File *f = GetFile(fd);
  if (f) {
    f->Close();
    delete f;
    map_.erase(fd);
    return 0;
  }
  load_functions();
  return real_close(fd);
}

off_t lseek(int fd, off_t offset, int whence) {
  moose::File *f = GetFile(fd);
  if (f)
    return f->Seek(offset, whence);

  load_functions();
  return real_close(fd);
}

void SetMountPoint(const char *pathname) {
  if (pathname) {
    mount_point_ = pathname;

    if (mount_point_[mount_point_.size() - 1] != '/')
      mount_point_ += '/';
  }
}

bool SetMaster(const char *host_ip) {
  if (!master_)
    master_ = new moose::MasterServer();

  load_functions();
  return master_->Connect(host_ip);
}
