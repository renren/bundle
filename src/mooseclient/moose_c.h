#ifndef MOOSECLIENT_MOOSE_C_H__
#define MOOSECLIENT_MOOSE_C_H__

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

extern "C" {

int mfs_stat(const char *path, struct stat *buf);
int mfs_open(const char *pathname, int flags);
int mfs_creat(const char *pathname, mode_t mode);
ssize_t mfs_read(int fd, void *buf, size_t count);
ssize_t mfs_write(int fd, const void *buf, size_t count);
ssize_t mfs_pread(int fd, void *buf, size_t count, off_t offset);
ssize_t mfs_pwrite(int fd, const void *buf, size_t count, off_t offset);
int mfs_close(int fd);
off_t mfs_lseek(int fd, off_t offset, int whence);

bool mfs_connect(const char *host_ip);

};

#endif
