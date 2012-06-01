#ifndef MOOSECLIENT_MOOSE_C_H__
#define MOOSECLIENT_MOOSE_C_H__

#include <sys/stat.h>
#include <sys/types.h>
#ifdef OS_LINUX
  #include <unistd.h>
#endif

#ifdef OS_WIN
  #include <stdint.h>
  typedef uint32_t mode_t;
  typedef __int64 ssize_t;
  #define F_OK 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

int mfs_stat(const char *path, struct stat *buf);
int mfs_open(const char *pathname, int flags, mode_t mode);
int mfs_creat(const char *pathname, mode_t mode);
ssize_t mfs_read(int fd, void *buf, size_t count);
ssize_t mfs_write(int fd, const void *buf, size_t count);
ssize_t mfs_pread(int fd, void *buf, size_t count, off_t offset);
ssize_t mfs_pwrite(int fd, const void *buf, size_t count, off_t offset);
int mfs_close(int fd);
off_t mfs_lseek(int fd, off_t offset, int whence);
int mfs_access(const char *pathname, int mode);
int mfs_unlink(const char *pathname);

int mfs_mkdir(const char *pathname, mode_t mode);
int mfs_rmdir(const char *pathname);
int mfs_mkdirs(const char *pathname, mode_t mode);

bool mfs_connect(const char *host_ip);
// void mfs_setmountpoint(const char *pathname);

#ifdef __cplusplus
};
#endif

#endif
