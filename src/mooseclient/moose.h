#ifndef MOOSECLIENT_MOOSE_H__
#define MOOSECLIENT_MOOSE_H__

#ifndef WIN32
  #include <stdint.h>
  #include <string.h>

  #include <sys/types.h>
  #include <sys/socket.h>
#else
  #include <boost/cstdint.hpp>
#endif

#include <sys/stat.h>
#include <vector>

#include "moosefs/MFSCommunication.h"

namespace moose {

struct Buffer;

#pragma pack(push)
#pragma pack(1)

struct FileAttribute {
  char type() const {
    return buf[0];
  }

  int mode() const {
    struct stat stbuf = {0};
    to_stat(0, &stbuf);
    return stbuf.st_mode;
  }

  uint64_t size() const {
    struct stat stbuf = {0};
    to_stat(0, &stbuf);
    return stbuf.st_size;
  }
  
  void to_stat(uint32_t inode, struct stat *stbuf) const;

  uint8_t buf[35];

  FileAttribute() {
    memset(buf, 0, sizeof(buf));
  }
};

#pragma pack(pop)


struct Location {
  uint32_t ip;
  uint16_t port;

  // Location() : ip(0), port(0) {}
  Location(uint32_t ip_ = 0, uint16_t port_ = 0) : ip(ip_), port(port_) {}
};

struct Chunk {
  uint64_t id;
  // uint64_t file_length;

  uint32_t version;
  typedef std::vector<Location> LocationType;
  LocationType location;

  Chunk() : id(0), version(0) {}
};

class MasterServer {
public:
  MasterServer() : master_socket_(0), id_(0)
    , session_id_(0)
    , uid_(0), gid_(0)
    , auto_reconnect_(true)
    , hostaddr_(0)
    , hostaddr_len_(0)
  {}

  ~MasterServer();

  // synchronized connect, and register
  bool Connect(const char *host_ip);
  bool Connect(struct sockaddr *addr, int addr_len, bool remember_address = true);

  int Lookup(uint32_t parent, const char *name, uint32_t *inode, FileAttribute *attr);
  int Lookup(const char *name, uint32_t *inode, FileAttribute *attr);

  int GetAttr(uint32_t inode, FileAttribute *attr);

  // keep opened=0
  int Truncate(uint32_t inode, uint8_t opened, uint64_t attrlength, FileAttribute *attr);
  // TODO: SetAttr, GetAttr

  int Access(uint32_t inode, uint8_t modemask);

  // WANT_READ | WANT_WRITE | WANT_CREATE
  int Open(uint32_t inode, uint8_t flags, FileAttribute *attr);

  // TYPE_FILE should use Create
  int Mknod(uint32_t parent, const char *name, uint8_t type, uint16_t mode, uint32_t rdev, uint32_t *inode, FileAttribute *attr);

  int Mkdir(uint32_t parent, const char *name, uint16_t mode, uint32_t *inode, FileAttribute *attr);
  int Rmdir(uint32_t parent, const char *name);

  int Create(uint32_t parent, const char *name, mode_t mode, uint32_t *inode);
  int Unlink(uint32_t parent, const char *name);
  
  int session_id() const {
    return session_id_;
  }

  int ReadChunk(uint32_t inode, uint32_t indx, uint64_t *length, Chunk *chunks);
  int WriteChunk(uint32_t inode, uint32_t indx, uint64_t *length, Chunk *chunks);
  int WriteChunkEnd(uint64_t chunkid, uint32_t inode, uint64_t length);
private:
  void EnsureConnect();
  void Disconnect();

  const uint8_t *SendAndReceive(struct Buffer *buf, uint32_t expect_cmd, int *left_length);
  bool ReadCommand(struct Buffer *input, uint32_t *cmd_, uint32_t *length_);

  bool BuildRegister(struct Buffer *output);

  bool GotRegister(uint32_t cmd, const uint8_t *payload, int length);

  // TODO:
  bool BuildQueryInfo(Buffer *output);
  
private:
  int master_socket_;

  int id_;
  uint32_t session_id_;
  int uid_, gid_;
  bool auto_reconnect_;
  char *hostaddr_;
  int hostaddr_len_;

  enum {
    VERSMAJ = 1,
    VERSMID = 6,
    VERSMIN = 24,
  };
};

class ChunkServer {
public:
  ChunkServer() : socket_(0), ip_(0), port_(0) {}

  ~ChunkServer();

  bool Connect(uint32_t ip, uint16_t port);

  int ReadBlock(Chunk *chunk,uint32_t offset,uint32_t size,uint8_t *buff);
  int WriteBlockInit(Chunk *chunk, uint32_t *writeid);
  int WriteBlock(Chunk *chunk,uint32_t writeid,uint16_t blockno,uint16_t offset,uint32_t size, const uint8_t *buff);

  uint32_t ip() const {
    return ip_;
  }
  uint16_t port() const {
    return port_;
  }

private:
  int socket_;

  uint32_t ip_;
  uint16_t port_;
};

class File {
public:
  File(MasterServer *m) : master_(m), position_(0), inode_(0), truncate_in_close_(false) {}

  uint32_t inode() const {
    return inode_;
  }

  int Open(const char *pathname, int flags, mode_t mode);
  int Create(const char *pathname, mode_t mode);
  int Close();

  uint32_t Write(const char *buf, size_t count);
  uint32_t Read(char *buf, size_t count);

  uint64_t Seek(uint64_t offset, int whence);
  uint64_t Tell() const {
    return position_;
  }

  uint32_t Pwrite(const char *buf, size_t count, off_t offset);
  uint32_t Pread(char *buf, size_t count, off_t offset);

private:
  uint32_t WriteInternal(const char *buf, size_t count);

  MasterServer* master_;
  uint64_t position_;
  uint32_t inode_;
  bool truncate_in_close_; // maybe truncate in File::Close
};

}
#endif
