#include "mooseclient/moose_c.h"

#include "mooseclient/moose.h"

#include <unordered_map>

moose::MasterServer *master_ = NULL;
typedef std::unordered_map<int, moose::File*> TrapedMapType;
TrapedMapType map_;

bool mfs_connect(const char *host_ip) {
  if (!master_)
    master_ = new moose::MasterServer();

  return master_->Connect(host_ip);
}

static moose::File* GetFile(int fd) {
  TrapedMapType::iterator i = map_.find(fd);
  if (i == map_.end())
    return NULL;
  return i->second;
}

int mfs_stat(const char *path, struct stat *buf) {
  if (!master_)
    return -1;

  moose::FileAttribute attr;
  uint32_t inode;
  int ret = master_->Lookup(path, &inode, &attr);
  if (!ret)
    return ret;

  if (buf)
    attr.to_stat(inode, buf);

  return 0;
}

int mfs_open(const char *pathname, int flags) {
  moose::File * f = new moose::File(master_);
  if (0 == f->Open(pathname, flags, 0)) {
    map_[f->inode()] = f;
    return f->inode();
  }

  delete f;
  return -1;
}

int mfs_creat(const char *pathname, mode_t mode) {
  moose::File * f = new moose::File(master_);
  if (0 == f->Create(pathname, mode)) {
    int fd = f->inode();
    map_[fd] = f;
    return fd;
  }

  delete f;
  return -1;
}

ssize_t mfs_read(int fd, void *buf, size_t count) {
  moose::File *f = GetFile(fd);
  if (f)
    return f->Read((char *)buf, count);
  return -1;
}

ssize_t mfs_write(int fd, const void *buf, size_t count) {
  moose::File *f = GetFile(fd);
  if (f) {
    return f->Write((const char *)buf, count);
  }
  return -1;
}

ssize_t mfs_pread(int fd, void *buf, size_t count, off_t offset) {
  moose::File *f = GetFile(fd);
  if (f) {
    return f->Pread((char *)buf, count, offset);
  }
  return -1;
}

ssize_t mfs_pwrite(int fd, const void *buf, size_t count, off_t offset) {
  moose::File *f = GetFile(fd);
  if (f) {
    return f->Pwrite((const char *)buf, count, offset);
  }
  return -1;
}

int mfs_close(int fd) {
  moose::File *f = GetFile(fd);
  if (!f)
    return -1;
  
  f->Close();
  delete f;
  map_.erase(fd);
  return 0;
}

off_t mfs_lseek(int fd, off_t offset, int whence) {
  moose::File *f = GetFile(fd);
  if (f)
    return f->Seek(offset, whence);

  return -1;
}

