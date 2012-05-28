#include "mooseclient/moose_c.h"

#include "base3/pathops.h"
#include "base3/hashmap.h"
#include "mooseclient/moose.h"

#include <errno.h>
#include <iostream>

static moose::MasterServer *master_ = NULL;
typedef std::hash_map<int, moose::File*> TrapedMapType;
static TrapedMapType map_;

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
  if (ret) {
    // return ret;
    if (ret == ERROR_ENOENT)
      errno = ENOENT;
    else
      errno = ret;
    return -1;
  }

  if (buf)
    attr.to_stat(inode, buf);

  return 0;
}

int mfs_open(const char *pathname, int flags, mode_t mode) {
  moose::File * f = new moose::File(master_);
  if (0 == f->Open(pathname, flags, mode)) {
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

int mfs_access(const char *pathname, int mode) {
  if (!master_)
    return -1;

  moose::FileAttribute attr;
  uint32_t inode;
  int ret = master_->Lookup(pathname, &inode, &attr);
  if (ret) {
    errno = ret;
    return -1;
  }

  // R_OK, W_OK, X_OK, F_OK
  // TODO: 
  if (F_OK == mode)
    return 0;
  return 0;
}

// parent inode, basename
//
int extract(const char *pathname, uint32_t *parent_inode, std::string *basename) {
  std::string dir = base::Dirname(pathname);
  if (basename)
    *basename = base::Basename(pathname);

  if (dir.empty()) {
    if (parent_inode)
      *parent_inode = MFS_ROOT_ID;

    return 0;
  }

  uint32_t inode;
  int ret = master_->Lookup(dir.c_str(), &inode, 0);
  if (0 == ret && parent_inode)
    *parent_inode = inode;
  
  return ret;
}

int mfs_unlink(const char *pathname) {
  if (!master_)
    return -1;

  uint32_t parent;
  std::string basename;
  int ret = extract(pathname, &parent, &basename);
  if (0 == ret)
    return master_->Unlink(parent, basename.c_str());
  return ret;
}

int mfs_mkdir(const char *pathname, mode_t mode) {
  if (!master_)
    return -1;

  uint32_t parent, inode;
  std::string basename;
  int ret = extract(pathname, &parent, &basename);
  if (0 != ret) {
    errno = ret;
    return -1;
  }

  ret = master_->Mkdir(parent, basename.c_str(), mode, &inode, NULL);
  if (ret) {
    if (ERROR_EEXIST == ret)
      errno = EEXIST;
    else
      errno = ret;
    return -1;
  }
  return 0;
}

int mfs_rmdir(const char *pathname) {
  if (!master_)
    return -1;

  uint32_t parent;
  std::string basename;
  int ret = extract(pathname, &parent, &basename);
  if (0 == ret) {
    return master_->Rmdir(parent, basename.c_str());
  }
  return ret;
}

int mfs_mkdirs(const char *pathname, mode_t mode) {
  int retval = 0;
#define PATH_LENGTH_MAX 300
  char path[PATH_LENGTH_MAX];

  strncpy(path, pathname, PATH_LENGTH_MAX - 1);
  path[PATH_LENGTH_MAX - 1] = '\0';

  size_t path_len = strlen(path);
  if (path_len && '/' == path[path_len - 1])
    path[path_len - 1] = '\0';

  std::string subpath;
  char *pa = path;
  if ('/' == path[0]) {
    subpath.append("/");
    pa++;
  }
  else if (('.' == path[0]) && ('/' == path[1])) {
    subpath.append("./");
    pa+=2;
  }

  char *p;
  while (1) {
    if (0 != (p = strchr(pa, '/'))) {
      *p = '\0';
      subpath.append(pa);
      if (0 != (retval = mfs_mkdir(subpath.c_str(), mode))) {
        if (EEXIST == errno) {
          p++;
          pa = p;
          subpath.append("/");
          continue;
        }
        else {
          return retval;
        }
      }
      else {
        p++;
        pa = p;
        subpath.append("/");
      }
    }
    // the last chars after '/'
    else {
      subpath.append(pa);
      if (0 != (retval = mfs_mkdir(subpath.c_str(), mode))) {
        if (EEXIST == errno) {
          return 0;
        }
        else {
          return retval;
        }
      }
      else {
        return 0;
      }
    }
  }

}
