#ifndef WIN32
  #include <unistd.h> // close/write

  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h> // IPPROTO_TCP
  #include <netinet/tcp.h> // TCP_NODELAY
#endif

#include <fcntl.h>
#include <time.h> // for time(0)
#include <signal.h>
#include <errno.h>

#include <stdlib.h>
#include <stdio.h> // perror
#include <string.h> // memset

#include <algorithm>
#include <iostream>
#include <iomanip>
#include "base3/hashmap.h"

#include "boost/static_assert.hpp"

#ifndef WIN32
  #include "moosefs/datapack.h"
#else
  #include "moosefs/datapack_win.h"
  #define O_ACCMODE   (O_RDONLY | O_WRONLY | O_RDWR)
  #define sleep(x) Sleep(1000 * x)
#endif

#include "mooseclient/sockutil.h"
#include "mooseclient/moose.h"

#ifndef ASSERT
#ifndef NDEBUG
#if defined(__GNUC__)
  #define ASSERT(x) if(!(x)) {printf("ASSERT failed %s %d\n", #x, __LINE__); __asm__("int $3");}
#elif defined(_MSC_VER)
  #define ASSERT(x) if(!(x)) __asm int 3;
#endif
#else // NDEBUG
#define ASSERT(x)
#endif // NDEBUG
#endif // ASSERT


#include "boost/crc.hpp"

uint32_t mycrc32(uint32_t crc,const uint8_t *block,uint32_t leng) {
  boost::crc_32_type p;
  p.process_bytes(block, leng);
  return p.checksum();
}

namespace moose {

struct Buffer {
  uint8_t *ptr;
  int used;
  int length;

  Buffer() : ptr(0), used(0), length(0) {}
  bool Alloca(int len) {
    if (len > length) {
      delete [] ptr;

      ptr = new uint8_t[len];
      length = len;
    }
    return true;
  }
  ~Buffer() {
    delete [] ptr;
  }
private:
  Buffer(const Buffer&);
  Buffer& operator= (const Buffer&);
};

static void mfs_attr_to_stat(uint32_t inode,const uint8_t attr[35], struct stat *stbuf) {
  uint16_t attrmode;
  uint8_t attrtype;
  uint32_t attruid,attrgid,attratime,attrmtime,attrctime,attrnlink,attrrdev;
  uint64_t attrlength;
  const uint8_t *ptr;
  ptr = attr;
  attrtype = get8bit(&ptr);
  attrmode = get16bit(&ptr);
  attruid = get32bit(&ptr);
  attrgid = get32bit(&ptr);
  attratime = get32bit(&ptr);
  attrmtime = get32bit(&ptr);
  attrctime = get32bit(&ptr);
  attrnlink = get32bit(&ptr);
  stbuf->st_ino = inode;
#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
  stbuf->st_blksize = 0x10000;
#endif
  switch (attrtype) {
  case TYPE_DIRECTORY:
    stbuf->st_mode = S_IFDIR | ( attrmode & 07777);
    attrlength = get64bit(&ptr);
    stbuf->st_size = attrlength;
#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
    stbuf->st_blocks = (attrlength+511)/512;
#endif
    break;
#ifndef WIN32
  case TYPE_SYMLINK:
    stbuf->st_mode = S_IFLNK | ( attrmode & 07777);
    attrlength = get64bit(&ptr);
    stbuf->st_size = attrlength;
#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
    stbuf->st_blocks = (attrlength+511)/512;
#endif
    break;
#endif
  case TYPE_FILE:
    stbuf->st_mode = S_IFREG | ( attrmode & 07777);
    attrlength = get64bit(&ptr);

    // CAUTION: in 32bit compile
    // -D_FILE_OFFSET_BITS=64
    stbuf->st_size = attrlength;
#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
    stbuf->st_blocks = (attrlength+511)/512;
#endif
    break;
#ifndef WIN32
  case TYPE_FIFO:
    stbuf->st_mode = S_IFIFO | ( attrmode & 07777);
    stbuf->st_size = 0;
#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
    stbuf->st_blocks = 0;
#endif
    break;
  case TYPE_SOCKET:
    stbuf->st_mode = S_IFSOCK | ( attrmode & 07777);
    stbuf->st_size = 0;
#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
    stbuf->st_blocks = 0;
#endif
    break;
  case TYPE_BLOCKDEV:
    stbuf->st_mode = S_IFBLK | ( attrmode & 07777);
    attrrdev = get32bit(&ptr);
#ifdef HAVE_STRUCT_STAT_ST_RDEV
    stbuf->st_rdev = attrrdev;
#endif
    stbuf->st_size = 0;
#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
    stbuf->st_blocks = 0;
#endif
    break;
  case TYPE_CHARDEV:
    stbuf->st_mode = S_IFCHR | ( attrmode & 07777);
    attrrdev = get32bit(&ptr);
#ifdef HAVE_STRUCT_STAT_ST_RDEV
    stbuf->st_rdev = attrrdev;
#endif
    stbuf->st_size = 0;
#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
    stbuf->st_blocks = 0;
#endif
    break;
#endif
  default:
    stbuf->st_mode = 0;
  }
  stbuf->st_uid = attruid;
  stbuf->st_gid = attrgid;
  stbuf->st_atime = attratime;
  stbuf->st_mtime = attrmtime;
  stbuf->st_ctime = attrctime;
#ifdef HAVE_STRUCT_STAT_ST_BIRTHTIME
  stbuf->st_birthtime = attrctime;	// for future use
#endif
  stbuf->st_nlink = attrnlink;
}

void FileAttribute::to_stat(uint32_t inode, struct stat *stbuf) const {
  return mfs_attr_to_stat(0, buf, stbuf);
}

int tcptoread(int sock, uint8_t *buff,uint32_t leng) {
  uint32_t rcvd=0;
  int i;

  while (rcvd<leng) {
    i = read(sock,((uint8_t*)buff)+rcvd,leng-rcvd);
    if (i<=0) {
      return i;
    }
    rcvd+=i;
  }
  return rcvd;
}

bool ReadCommand(int fd, struct Buffer *input, uint32_t *cmd_, uint32_t *length_) {
  input->Alloca(8);

  uint32_t cmd = ANTOAN_NOP;
  uint32_t length = 0;
  while (cmd == ANTOAN_NOP) {
    int ret = tcptoread(fd, input->ptr, 8);
    if (ret <= 0) {
      std::cerr << "tcptoread " << fd << " failed " << errno << std::endl;
      return false;
    }
    
    if (8 != ret)
      return false;

    const uint8_t *mem = input->ptr;

    cmd = get32bit(&mem);
    length = get32bit(&mem);
  
    if (cmd == ANTOAN_NOP) {
      // eat here
      ASSERT(length == 4);
      ret = tcptoread(fd, input->ptr, 4);
      ASSERT(ret == 4);
    }
  }

  if (length > 64 *1024 *1024) {
    std::cerr << " unexpected length " << length << std::endl;
    return false;
  }

  if (cmd_)
    *cmd_ = cmd;

  if (length_)
    *length_ = length;

  input->Alloca(8 + length);

  uint32_t ret = tcptoread(fd, input->ptr + 8, length);
  if (ret == length)
    return true;
  else
    std::cerr << " unexpected tcptoread " << ret << std::endl;
  return false;
}

uint8_t *BuildCommand(struct Buffer *output, uint32_t cmd, uint32_t length, uint32_t session_id) {
  output->Alloca(8 + 4 + length);

  uint8_t *wptr = output->ptr;
  put32bit(&wptr, cmd);
  put32bit(&wptr, 4 + length);
  put32bit(&wptr, session_id);

  return wptr;
}

MasterServer::~MasterServer() {
  if (master_socket_)
    close(master_socket_);
  delete [] hostaddr_;
}

bool MasterServer::Connect(const char *host_ip) {
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  int outlen = sizeof(addr);
  if (0 == parse_sockaddr_port(host_ip, (struct sockaddr*)&addr, &outlen))
    return Connect((struct sockaddr*)&addr, outlen);
  return false;
}

  // synchronized connect, and register
bool MasterServer::Connect(struct sockaddr *addr, int addr_len, bool remember_address) {
  ASSERT(0 == master_socket_);
  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  ASSERT(sock > 0);
  if (sock < 0) {
    perror("socket");
    return false;
  }

  {
    // TODO: timeout
    int yes=1;
    // setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&yes, sizeof(int));
  }

  int ret = connect(sock, addr, addr_len);
  ASSERT(ret == 0);
  if (ret < 0) {
    perror("connect to master failed");

    close(sock);
    return false;
  }

  Buffer buf;
  ret = BuildRegister(&buf);
  if (!ret) {
    std::cerr << "BuildRegister failed\n";
    close(sock);
    return false;
  }

  ret = write(sock, buf.ptr, buf.used);
  ASSERT(ret == buf.used);
  if (0 == ret) {
    perror("write");
    close(sock);
    return false;
  }

  uint32_t cmd = 0, length = 0;
  ret = moose::ReadCommand(sock, &buf, &cmd, &length);
  if (!ret) {
    close(sock);
    return false;
  }
  ASSERT(cmd == MATOCU_FUSE_REGISTER);

  // TODO: avoid magic 8
  ret = GotRegister(cmd, buf.ptr + 8, length);
  ASSERT(session_id_);

  if (remember_address) {
    hostaddr_ = new char[addr_len];
    memcpy(hostaddr_, addr, addr_len);
    hostaddr_len_ = addr_len;
  }
  master_socket_ = sock;
  return true;
}

static time_t last_registered_ = 0;

void MasterServer::EnsureConnect() {
  if (master_socket_)
    return;

  if (!hostaddr_ || !auto_reconnect_)
    return;

  
  int retry = 10;
  do {
    bool f = Connect((struct sockaddr*)hostaddr_, hostaddr_len_, false);
    if (f)
      break;
    std::cerr << "EnsureConnect retry once left " << retry << std::endl;
    sleep(1);
  } while (retry-- > 0);

  if (retry != 10)
    last_registered_ = time(0);
}

void MasterServer::Disconnect() {
  if (master_socket_) {
#ifdef OS_LINUX
    shutdown(master_socket_, SHUT_RDWR);
#else
    shutdown(master_socket_, SD_BOTH);
#endif

    close(master_socket_);

    master_socket_ = 0;
  }
}

bool MasterServer::BuildRegister(struct Buffer *output) {
  const char info[] = "/libmoose/mount";
  const char subfolder[] = "/";

  int ileng = sizeof(info);
  int pleng = sizeof(subfolder);

  const int message_length = 8 + 64 + 13 + ileng + pleng;

  if (!output->Alloca(message_length)) {
    // TODO: LOG error
    return false;
  }

  uint8_t *wptr = output->ptr;

  put32bit(&wptr, CUTOMA_FUSE_REGISTER);

  // TODO: password
  if (0 == session_id_) {
    put32bit(&wptr, 64 + 13 + ileng + pleng);
  } else {
    put32bit(&wptr, 64 + 1 + 4 + 4);
  }

  memcpy(wptr, FUSE_REGISTER_BLOB_ACL, 64);
  wptr+=64;

  // ACL.2 103 REGISTER_NEWSESSION
  // ACL.3 73  REGISTER_RECONNECT

  if (0 == session_id_) {
    // TODO: REGISTER_NEWMETASESSION
    put8bit(&wptr,REGISTER_NEWSESSION);
  } else {
    put8bit(&wptr,REGISTER_RECONNECT);
    put32bit(&wptr, session_id_);
  }

  put16bit(&wptr,VERSMAJ);
  put8bit(&wptr,VERSMID);
  put8bit(&wptr,VERSMIN);

  if (0 == session_id_) {
    put32bit(&wptr,ileng);
    memcpy(wptr,info,ileng);
    wptr+=ileng;
    // if !meat
    put32bit(&wptr,pleng);
    memcpy(wptr,subfolder,pleng);
    wptr+=pleng;
  }

  output->used = wptr - output->ptr;
  return true;
}

bool MasterServer::GotRegister(uint32_t, const uint8_t *payload, int) {
  // TDOO: session_flags, uid, gid, map uid, map gid
  uint32_t sid = get32bit(&payload);
  if (session_id_ == 0)
    session_id_ = sid;
  else
    ASSERT(sid == session_id_);
  return true;
}

bool MasterServer::BuildQueryInfo(Buffer *output) {
  const int message_length = 8;
  if (!output->Alloca(message_length)) {
    return false;
  }

  uint8_t *wptr = output->ptr;
  put32bit(&wptr, CUTOMA_INFO);
  put32bit(&wptr, 0);
  output->used = wptr - output->ptr;

  return true;
}

int MasterServer::Lookup(uint32_t parent, const char *name, uint32_t *inode, FileAttribute *attr) {
  int nleng = strlen(name);

  Buffer buf;
  uint8_t *wptr = BuildCommand(&buf, CUTOMA_FUSE_LOOKUP, 13 + nleng, session_id_);

  put32bit(&wptr,parent);
  put8bit(&wptr,nleng);
  memcpy(wptr,name,nleng);
  wptr+=nleng;
  put32bit(&wptr,uid_);
  put32bit(&wptr,gid_);

  buf.used = wptr - buf.ptr;

  int length;
  const uint8_t *rptr = SendAndReceive(&buf, MATOCU_FUSE_LOOKUP, &length);
  if (!rptr) return ERROR_IO;

  if (length == 1) {
    uint8_t status = get8bit(&rptr);
    return status;
  }

  if (length == 39) {
    uint32_t t = get32bit(&rptr);
    if (inode)
      *inode = t;
    if (attr)
      memcpy(attr, rptr, 35);
    return STATUS_OK;
  }

  // TODO: remove
  std::cerr << "lookup " << name << " unexpected size " << length << std::endl;
  Disconnect();
  return ERROR_IO;
}

int MasterServer::Lookup(const char *name, uint32_t *inode, FileAttribute *attr) {
  uint32_t inode_temp = MFS_ROOT_ID;
  FileAttribute attr_temp;

  int ret;

  char *to_free = strdup(name);
  char *copy = to_free;
  char *slash = copy;
  do {
    slash = strchr(copy, '/');

    if (slash) {
      *slash = '\0';
    }

    if (slash != copy && copy[0] != '\0') {
      uint32_t inode_parent = inode_temp;
      ret = Lookup(inode_parent, copy, &inode_temp, &attr_temp);
      if (ret != STATUS_OK) {
        free(to_free);
        return ret;
      }
    }

    copy = slash + 1;
  } while (slash);

  free(to_free);

  if (inode)
    *inode = inode_temp;

  if (attr)
    *attr = attr_temp;
  return STATUS_OK;
}

int MasterServer::GetAttr(uint32_t inode, FileAttribute *attr) {
  Buffer buf;
  uint8_t *wptr = BuildCommand(&buf, CUTOMA_FUSE_GETATTR, 12, session_id_);
  if (wptr==NULL) {
    return ERROR_IO;
  }
  put32bit(&wptr,inode);
  put32bit(&wptr,uid_);
  put32bit(&wptr,gid_);

  buf.used = wptr - buf.ptr;

  int ret = STATUS_OK;
  int length;
  const uint8_t *rptr = SendAndReceive(&buf, MATOCU_FUSE_GETATTR, &length);
  if (rptr==NULL) {
    ret = ERROR_IO;
  } else if (length==1) {
    ret = rptr[0];
  } else if (length!=35) {
    Disconnect();
    ret = ERROR_IO;
  } else {
    memcpy(attr,rptr,35);
    ret = STATUS_OK;
  }
  return ret;
}

int MasterServer::Truncate(uint32_t inode, uint8_t opened, uint64_t attrlength, FileAttribute *attr) {
  Buffer buf;
  uint8_t *wptr = BuildCommand(&buf, CUTOMA_FUSE_TRUNCATE, 21, session_id_);
  put32bit(&wptr,inode);
  put8bit(&wptr,opened);
  put32bit(&wptr,uid_);
  put32bit(&wptr,gid_);
  put64bit(&wptr,attrlength);

  buf.used = wptr - buf.ptr;

  int length;
  const uint8_t *rptr = SendAndReceive(&buf, MATOCU_FUSE_TRUNCATE, &length);
  if (!rptr) return ERROR_IO;

  if (length == 1)
    return rptr[0] == STATUS_OK;

  if (length == 35 && attr) {
    memcpy(attr, rptr, 35);
    return STATUS_OK;
  }

  Disconnect();
  return ERROR_IO;
}

int MasterServer::Access(uint32_t inode, uint8_t modemask) {
  Buffer buf;
  uint8_t *wptr = BuildCommand(&buf, CUTOMA_FUSE_ACCESS, 13, session_id_);
  put32bit(&wptr,inode);
  put32bit(&wptr,uid_);
  put32bit(&wptr,gid_);
  put8bit(&wptr,modemask);

  int ret = STATUS_OK;
  int length;
  const uint8_t *rptr = SendAndReceive(&buf, MATOCU_FUSE_ACCESS, &length);
  if (!rptr || length!=1) {
    ret = ERROR_IO;
  } else {
    ret = rptr[0];
  }
  return ret;
}

// WANT_READ | WANT_WRITE | WANT_CREATE
int MasterServer::Open(uint32_t inode, uint8_t flags, FileAttribute *attr) {
  Buffer buf;
  uint8_t *wptr = BuildCommand(&buf, CUTOMA_FUSE_OPEN, 13, session_id_);
  put32bit(&wptr,inode);
  put32bit(&wptr,uid_);
  put32bit(&wptr,gid_);
  put8bit(&wptr,flags);

  buf.used = wptr - buf.ptr;
  int length;
  const uint8_t *rptr = SendAndReceive(&buf, MATOCU_FUSE_OPEN, &length);
  if (!rptr)
    return ERROR_IO;

  if (length == 1)
    return rptr[0];

  if (length == 35 && attr) {
    memcpy(attr, rptr, 35);
    return STATUS_OK;
  }

  Disconnect();
  return ERROR_IO;
}

int MasterServer::Mknod(uint32_t parent, const char *name, uint8_t type, uint16_t mode, uint32_t rdev, uint32_t *inode, FileAttribute *attr) {
  int nleng = strlen(name);

  Buffer buf;
  uint8_t *wptr = BuildCommand(&buf, CUTOMA_FUSE_MKNOD, 20 + nleng, session_id_);
  put32bit(&wptr,parent);
  put8bit(&wptr,nleng);
  memcpy(wptr,name,nleng);
  wptr+=nleng;
  put8bit(&wptr,type);
  put16bit(&wptr,mode);
  put32bit(&wptr,uid_);
  put32bit(&wptr,gid_);
  put32bit(&wptr,rdev);

  buf.used = wptr - buf.ptr;

  int length;
  const uint8_t *rptr = SendAndReceive(&buf, MATOCU_FUSE_MKNOD, &length);
  if (!rptr)
    return ERROR_IO;

  if (length == 1) {
    return rptr[0];
  }

  if (length != 39) {
    return ERROR_IO;
  }

  uint32_t t = get32bit(&rptr);
  if (inode)
    *inode = t;
  // TODO: cache attribute or filename -> inode
  if (attr)
    memcpy(attr, rptr, 35);
  return STATUS_OK;
}

int MasterServer::Mkdir(uint32_t parent, const char *name, uint16_t mode, uint32_t *inode, FileAttribute *attr) {
  int nleng = strlen(name);

  Buffer buf;
  uint8_t *wptr = BuildCommand(&buf, CUTOMA_FUSE_MKDIR, 15 + nleng, session_id_);
  put32bit(&wptr,parent);
  put8bit(&wptr,nleng);
  memcpy(wptr,name,nleng);
  wptr+=nleng;
  put16bit(&wptr,mode);
  put32bit(&wptr,uid_);
  put32bit(&wptr,gid_);

  buf.used = wptr - buf.ptr;

  int length;
  const uint8_t *rptr = SendAndReceive(&buf, MATOCU_FUSE_MKDIR, &length);
  if (!rptr)
    return ERROR_IO;

  if (length == 1)
    return rptr[0];

  if (length != 39) {
    Disconnect();
    return ERROR_IO;
  }

  uint32_t t = get32bit(&rptr);
  if (inode)
    *inode = t;
  // TODO: cache attribute or filename -> inode
  if (attr)
    memcpy(attr, rptr, 35);
  return STATUS_OK;
}

int MasterServer::Rmdir(uint32_t parent, const char *name) {
  int nleng = strlen(name);

  Buffer buf;
  uint8_t *wptr = BuildCommand(&buf, CUTOMA_FUSE_RMDIR, 13 + nleng, session_id_);
  put32bit(&wptr,parent);
  put8bit(&wptr,nleng);
  memcpy(wptr,name,nleng);
  wptr+=nleng;
  put32bit(&wptr,uid_);
  put32bit(&wptr,gid_);

  buf.used = wptr - buf.ptr;

  int length;
  const uint8_t *rptr = SendAndReceive(&buf, MATOCU_FUSE_RMDIR, &length);
  if (!rptr)
    return ERROR_IO;

  if (length == 1)
    return rptr[0];

  Disconnect();
  return ERROR_IO;
}

int MasterServer::Unlink(uint32_t parent, const char *name) {
  int nleng = strlen(name);

  Buffer buf;
  uint8_t *wptr = BuildCommand(&buf, CUTOMA_FUSE_UNLINK, 13 + nleng, session_id_);
  put32bit(&wptr,parent);
  put8bit(&wptr,nleng);
  memcpy(wptr,name,nleng);
  wptr+=nleng;
  put32bit(&wptr,uid_);
  put32bit(&wptr,gid_);

  buf.used = wptr - buf.ptr;

  int length = 0;
  const uint8_t *rptr = SendAndReceive(&buf, MATOCU_FUSE_UNLINK, &length);
  if (!rptr) return ERROR_IO;
  if (length == 1)
    return rptr[0];

  Disconnect();
  return ERROR_IO;
}

int MasterServer::Create(uint32_t parent, const char *name, mode_t mode, uint32_t *inode) {
  // fs_mknod(parent,nleng,(const uint8_t*)name,TYPE_FILE,mode&07777,ctx->uid,ctx->gid,0,&inode,attr);
  return Mknod(parent, name, TYPE_FILE, mode & 07777, 0, inode, NULL);
}

class Cached {
public:
  static ChunkServer *Create(uint32_t ip, uint16_t port, bool is_read) {
    if (is_read) {
      uint64_t key = Key(ip, port, is_read);
      MapType::iterator i = map_.find(key);
      if (i != map_.end()) {
        map_.erase(i);
        return i->second;
      }
    }

    ChunkServer *cs = new ChunkServer;
    // std::cerr << " new cs:" << ip << ":" << port << std::endl;
    if (!cs->Connect(ip, port)) {
      delete cs;
      return 0;
    }
    return cs;
  }

  // ugly
  static uint64_t Key(uint32_t ip, uint16_t port, bool is_read) {
    return (uint64_t)ip << 32 | (uint32_t)port << 16 | (uint64_t)is_read;
  }

  static void Return(ChunkServer *cs, bool is_read) {
    if (is_read) {
      uint64_t key = Key(cs->ip(), cs->port(), is_read);
      MapType::iterator i = map_.find(key);
      if (i == map_.end()) {
        map_.insert(std::make_pair(key, cs));
        return;
      }
    }
    
    delete cs;
  }

  // TODO:
  typedef std::hash_map<uint64_t, ChunkServer*> MapType; 
  static MapType map_;
};

Cached::MapType Cached::map_;

const uint8_t *MasterServer::SendAndReceive(struct Buffer *buf, uint32_t expect_cmd, int *left_length) {
  EnsureConnect();

  ASSERT(buf->used > 0);
  int ret = 0;

  uint32_t cmd, length;
  int retry = 3;
  do {
    ret = write(master_socket_, buf->ptr, buf->used);
    if (!ret) {
      Disconnect();
      std::cerr << "retry [write] once\n";
      EnsureConnect();
      continue;
    }
    ASSERT(ret == buf->used);

    ret = moose::ReadCommand(master_socket_, buf, &cmd, &length);
    if (!ret) {
      Disconnect();
      std::cerr << "retry [read] once\n";
      EnsureConnect();
      continue;
    }
    break;
  } while (retry-- > 0);

  if (ret && cmd == expect_cmd) {
    const uint8_t *rptr = buf->ptr + 8;
    uint32_t packet_id = get32bit(&rptr);
    ASSERT(packet_id == session_id_);

    if (left_length)
      *left_length = length - 4;
    return rptr;
  }
  return NULL;
}

bool MasterServer::ReadCommand(struct Buffer *input, uint32_t *cmd_, uint32_t *length_) {  
  return moose::ReadCommand(master_socket_, input, cmd_, length_);
}

int MasterServer::ReadChunk(uint32_t inode, uint32_t indx, uint64_t *file_length, Chunk *chunk) {
  Buffer buf;
  uint8_t *wptr = BuildCommand(&buf, CUTOMA_FUSE_READ_CHUNK, 8, session_id_);
  put32bit(&wptr,inode);
  put32bit(&wptr,indx);

  buf.used = wptr - buf.ptr;

  int cmd, length;
  const uint8_t *rptr = SendAndReceive(&buf, MATOCU_FUSE_READ_CHUNK, &length);
  if (!rptr) return ERROR_IO;
  if (length == 1)
    return rptr[0];

  uint64_t t64 = get64bit(&rptr);
  if (file_length)
    *file_length = t64;

  t64 = get64bit(&rptr);
  uint32_t t32 = get32bit(&rptr);
  if (chunk) {
    chunk->id = t64;
    chunk->version = t32;

    int left = length - 8 - 8 - 4;

    while (left > 0) {
      Location loc;
      loc.ip = get32bit(&rptr);
      loc.port = get16bit(&rptr);
      chunk->location.push_back(loc);

      left -= 4 + 2;
    }
  }

  return STATUS_OK;
}

int MasterServer::WriteChunk(uint32_t inode, uint32_t indx, uint64_t *file_length, Chunk *chunk) {
  Buffer buf;
  uint8_t *wptr = BuildCommand(&buf, CUTOMA_FUSE_WRITE_CHUNK, 8, session_id_);
  put32bit(&wptr,inode);
  put32bit(&wptr,indx);

  buf.used = wptr - buf.ptr;

  int length;
  const uint8_t *rptr = SendAndReceive(&buf, MATOCU_FUSE_WRITE_CHUNK, &length);
  if (!rptr) return ERROR_IO;

  if (length == 1)
    return rptr[0];
  else if (length < 20 || (length - 20) % 6 != 0) {
    Disconnect();
    return ERROR_IO;
  }

  uint64_t t64 = get64bit(&rptr);
  if (file_length)
    *file_length = t64;

  t64 = get64bit(&rptr);
  uint32_t t32 = get32bit(&rptr);
  if (chunk) {
    chunk->id = t64;
    chunk->version = t32;

    int left = length - 8 - 8 - 4;

    while (left > 0) {
      Location loc;
      loc.ip = get32bit(&rptr);
      loc.port = get16bit(&rptr);
      chunk->location.push_back(loc);

      left -= 4 + 2;
    }
  }

  return STATUS_OK;
}

int MasterServer::WriteChunkEnd(uint64_t chunkid, uint32_t inode, uint64_t length) {
  Buffer buf;
  uint8_t *wptr = BuildCommand(&buf, CUTOMA_FUSE_WRITE_CHUNK_END, 20, session_id_);
  put64bit(&wptr,chunkid);
  put32bit(&wptr,inode);
  put64bit(&wptr,length);

  buf.used = wptr - buf.ptr;

  int length_;
  const uint8_t *rptr = SendAndReceive(&buf, MATOCU_FUSE_WRITE_CHUNK_END, &length_);
  if (!rptr) return ERROR_IO;

  if (length_ == 1)
    return rptr[0];

  return ERROR_IO;
}



bool ChunkServer::Connect(uint32_t ip, uint16_t port) {
  ASSERT(0 == socket_);
  socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  // ASSERT(socket_ > 0);
  if (socket_ < 0) {
    perror("socket");
    return false;
  }

  struct sockaddr_in sin;
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
  sin.sin_addr.s_addr = htonl(ip);

  int ret = connect(socket_, (struct sockaddr *)&sin, sizeof(sockaddr_in));
  ASSERT(ret == 0);
  if (ret < 0) {
    perror("chunk server connect failed");

    close(socket_);
    socket_ = 0;

    return false;
  }
  ip_ = ip;
  port_ = port;
  return true;
}

ChunkServer::~ChunkServer() {
  if (socket_) {
#ifdef OS_LINUX
    shutdown(socket_, SHUT_RDWR);
#else
    shutdown(socket_, SD_BOTH);
#endif
    close(socket_);
  }
}

int ChunkServer::ReadBlock(Chunk *chunk,uint32_t offset,uint32_t size,uint8_t *buff) {
  uint8_t *wptr,ibuff[28];
  const uint8_t *rptr;

  wptr = ibuff;
  put32bit(&wptr,CUTOCS_READ);
  put32bit(&wptr,20);
  put64bit(&wptr,chunk->id);
  put32bit(&wptr,chunk->version);
  put32bit(&wptr,offset);
  put32bit(&wptr,size);

  int ret = write(socket_, ibuff, 28);
  if (ret != 28) {
    perror("chunk server write");
    return ERROR_IO;
  }

  Buffer input;
  input.Alloca(20);

  for (;;) {
    // detached package should not use ReadCommand
    int ret = tcptoread(socket_, input.ptr, 8);
    ASSERT(ret == 8);
    if (ret != 8) {
      perror("chunk server read");
      return ERROR_IO;
    }

    rptr = input.ptr;
    uint32_t cmd = get32bit(&rptr);
    uint32_t length = get32bit(&rptr);
    if (cmd==CSTOCU_READ_STATUS) {
      if (length!=9) {
        // LOG(LOG_NOTICE,"readblock; READ_STATUS incorrect message size (%"PRIu32"/9)",l);
        return -1;
      }
      if (tcptoread(socket_,input.ptr,9)!=9) {
        // LOG(LOG_NOTICE,"readblock; READ_STATUS tcpread error: %s",strerr(errno));
        return -1;
      }
      rptr = input.ptr;
      uint64_t t64 = get64bit(&rptr);
      if (*rptr!=0) {
        // LOG(LOG_NOTICE,"readblock; READ_STATUS status: %"PRIu8,*rptr);
        return -1;
      }
      if (t64!=chunk->id) {
        // LOG(LOG_NOTICE,"readblock; READ_STATUS incorrect chunkid (got:%"PRIu64" expected:%"PRIu64")",t64,chunkid);
        return -1;
      }
      if (size!=0) {
        // LOG(LOG_NOTICE,"readblock; READ_STATUS incorrect data size (left: %"PRIu32")",size);
        return -1;
      }
      return 0;
    }
    else if (cmd == CSTOCU_READ_DATA) {
      ASSERT(length >= 20);
      if (length < 20) {
        // LOG(WARNING) << "READ_DATA incorrect message size:" << 
        return -1;
      }

      input.Alloca(20);
      ret = tcptoread(socket_, input.ptr, 20);
      if (20 != ret) {
        // LOG(WARNING) << "READ_DATA tcpread error
        return -1;
      }

      rptr = input.ptr;

      uint64_t t64 = get64bit(&rptr);
      if (t64 != chunk->id) {
        // LOG(WARNING) << "READ_DATA incorrect chunkid got:" << t64 << " excepted:" << chunkid;
        return -1;
      }
      uint16_t blockno = get16bit(&rptr);
      uint16_t blockoffset = get16bit(&rptr);
      uint32_t blocksize = get32bit(&rptr);
      uint32_t blockcrc = get32bit(&rptr);

      if (length != 20 + blocksize) {
        // LOG(WARNING) << READ_DATA incorrect message size (%"PRIu32"/%"PRIu32")",l,20+blocksize);
        return -1;
      }
      if (0 == blocksize) {
        // LOG(WARNING) << "READ_DATA empty block";
        return -1;
      }
      if (blockno!=(offset>>16)) {
        // LOG(WARNING) << "READ_DATA incorrect block number (got:%"PRIu16" expected:%"PRIu32")",blockno,(offset>>16));
        return -1;
      }
      if (blockoffset!=(offset&0xFFFF)) {
        // LOG(WARNING) << "READ_DATA incorrect block offset (got:%"PRIu16" expected:%"PRIu32")",blockoffset,(offset&0xFFFF));
        return -1;
      }

      uint32_t breq = 65536 - (uint32_t)blockoffset;
      if (size < breq) {
        breq = size;
      }
      if (blocksize!=breq) {
        // LOG(WARNING) << "READ_DATA incorrect block size (got:%"PRIu32" expected:%"PRIu32")",blocksize,breq);
        return -1;
      }
    
      uint32_t readed = 0;
#if 1
      readed = tcptoread(socket_, buff, blocksize);
#else 
      while (readed < blocksize) {
        ret = tcptoread(socket_, buff, blocksize);
        if (ret < 0)
          return ret;

        readed += ret;
        buff += ret;
        blocksize -= ret;
      }
#endif
      
      if (readed != blocksize) {
        // LOG
        return -1;
      }

      // crc32
      if (blockcrc != mycrc32(0, buff, blocksize)) {
        // LOG crc checksum error
        std::cout << "crc32 error result:" << std::hex << mycrc32(0, buff, blocksize)
          << " expect " << std::hex << blockcrc << std::endl;
        return -1;
      }

      offset += blocksize;
      size -= blocksize;
      buff += blocksize;
    }
  }

  return STATUS_OK;
}

int ChunkServer::WriteBlockInit(Chunk *chunk, uint32_t *writeid) {
  Buffer buf;

  uint32_t chainsize = (chunk->location.size() - 1) *6;

  buf.Alloca(8 + 12 + chainsize);

  uint8_t *wptr = buf.ptr;
  put32bit(&wptr,CUTOCS_WRITE);
  put32bit(&wptr,12+chainsize);
  put64bit(&wptr,chunk->id);
  put32bit(&wptr,chunk->version);
  
  for (size_t i=1; i<chunk->location.size(); ++i) {
    put32bit(&wptr, chunk->location[i].ip);
    put16bit(&wptr, chunk->location[i].port);
  }

  buf.used = wptr - buf.ptr;
  int ret = write(socket_, buf.ptr, buf.used);
  ASSERT(ret == buf.used);

  buf.Alloca(8 + 13);
  ret = tcptoread(socket_, buf.ptr, 8 + 13);
  if (ret != 8 + 13)
    return ERROR_IO;

  const uint8_t *rptr = buf.ptr;

  uint32_t cmd = get32bit(&rptr);
  uint32_t length = get32bit(&rptr);

  ASSERT(CSTOCU_WRITE_STATUS == cmd);
  ASSERT(13 == length);

  uint64_t recchunkid = get64bit(&rptr);
  uint32_t recwriteid = get32bit(&rptr);
  uint8_t recstatus = get8bit(&rptr);
  ASSERT(recchunkid == chunk->id);
  
  if (recstatus != STATUS_OK)
    return recstatus;

  *writeid = recwriteid;
  return STATUS_OK;
}

int ChunkServer::WriteBlock(Chunk *chunk, uint32_t writeid,uint16_t blockno,uint16_t offset,uint32_t size,const uint8_t *buff) {
  // #
  {
    uint8_t *ptr,ibuff[32];
    uint32_t crc;
    ptr = ibuff;
    put32bit(&ptr,CUTOCS_WRITE_DATA);
    put32bit(&ptr,24+size);
    put64bit(&ptr,chunk->id);
    put32bit(&ptr,writeid);
    put16bit(&ptr,blockno);
    put16bit(&ptr,offset);
    put32bit(&ptr,size);
    crc = mycrc32(0,buff,size);
    put32bit(&ptr,crc);

    // std::cerr << " CUTOCS_WRITE_DATA cid:" << chunk->id << std::endl;

    int ret = write(socket_, ibuff, 32);
    if (ret != 32) {
      std::cerr << "  WriteBlock write 1st failed ret: " << ret << " errno: " << errno << std::endl;
      return errno;
    }

    ret = write(socket_, buff, size);
    if (ret != size) {
      std::cerr << "  WriteBlock write 2nd failed ret: " << ret << " errno: " << errno << std::endl;
      return errno;
    }
  }

  Buffer buf;
  buf.Alloca(8 + 13);
  int ret = tcptoread(socket_, buf.ptr, 8 + 13);
  if (ret != 8 + 13)
    return ERROR_IO;

  const uint8_t *rptr = buf.ptr;

  uint32_t cmd = get32bit(&rptr);
  uint32_t length = get32bit(&rptr);

  ASSERT(CSTOCU_WRITE_STATUS == cmd);
  ASSERT(13 == length);

  uint64_t recchunkid = get64bit(&rptr);
  uint32_t recwriteid = get32bit(&rptr);
  uint8_t recstatus = get8bit(&rptr);

  // TODO: not match here
  // ASSERT(recwriteid == writeid);
  ASSERT(recchunkid == chunk->id);

  if (recstatus != STATUS_OK)
    return recstatus;

  return STATUS_OK;
}


int File::Open(const char*pathname, int flags, mode_t mode) {
  ASSERT(!inode_);
  ASSERT(master_);
  uint32_t inode;
  int ret = master_->Lookup(pathname, &inode, NULL);
  if (ERROR_ENOENT == ret && (flags & O_CREAT)) {
    return Create(pathname, mode);
  }

  // hack
  // TODO: check O_CREAT
  if (0 == ret && flags & O_EXCL) {
    return -1;
  }

  int oflags = 0;

  if ((flags & O_ACCMODE) == O_RDONLY) {
    oflags |= WANT_READ;
  } else if ((flags & O_ACCMODE) == O_WRONLY) {
    oflags |= WANT_WRITE;
    truncate_in_close_ = true;
  } else if ((flags & O_ACCMODE) == O_RDWR) {
    oflags |= WANT_READ | WANT_WRITE;
  }

  FileAttribute att;
  ret = master_->Open(inode, oflags, &att);
  if (ret != STATUS_OK)
    return ret;

  if ((flags & O_APPEND) == O_APPEND) {
    position_ = att.size();
    truncate_in_close_ = true;
  }
  else
    position_ = 0;

  inode_  = inode;
  return STATUS_OK;
}

int File::Create(const char *pathname, mode_t mode) {
  ASSERT(!inode_);
  ASSERT(master_);

  const char *name = NULL;
  // get parent
  uint32_t parent = 0;
  const char *slash = strrchr(pathname, '/');
  size_t len = strlen(pathname);
  if (slash && slash != pathname + len) {
    std::string parentname(pathname, slash - pathname);
    // std::cout << "loop up " << parentname << std::endl;
    int ret = master_->Lookup(parentname.c_str(), &parent, NULL);
    if (STATUS_OK != ret)
      return ret;

    name = slash + 1;
  }
  else {
    parent = MFS_ROOT_ID;
    name = pathname;
  }

  uint32_t inode = 0;
  int ret = master_->Create(parent, name, mode, &inode);
  if (ret != STATUS_OK)
    return ret;

  inode_ = inode;
  position_ = 0;
  return STATUS_OK;
}

int File::Close() {
  if (truncate_in_close_) {
    FileAttribute attr;
    int ret = master_->GetAttr(inode_, &attr);
    if (STATUS_OK == ret && attr.size() > position_) {
      FileAttribute attr2;
      ret = master_->Truncate(inode_, 0, position_, &attr2);
      if (STATUS_OK != ret)
        return ret;
      ASSERT(attr2.size() == position_);
    }
  }
  inode_ = 0;
  position_ = 0;
  return 0;
}

const int M64 = 64 *1024 *1024;
const int K64 = 64 *1024;

uint32_t File::Write(const char *buf, size_t count) {
  // split into 64K once 
  int ret = 0;
  while (count > 0) {
    int part = K64 - (position_ + K64) % K64;
    part = std::min(K64, (int)count);
    int written = WriteInternal(buf, part);
    if (written == part) {
      buf += part;
      count -= part;

      ret += written;
    }
    else
      return 0;
  }
  return ret;
}

uint32_t File::WriteInternal(const char *buf, size_t count) {
  ASSERT(inode_);  
  uint64_t end = position_ + count;

  uint64_t write_position = position_;
  int chunk_index = write_position >> 26;
  while (true) {
    uint64_t file_length;
    Chunk chunk;
    int ret = master_->WriteChunk(inode_, chunk_index, &file_length, &chunk);
    if (ret != STATUS_OK) {
      std::cerr << "WriteChunk failed " << ret 
        << " chunk index: " << chunk_index
        << " offset: " << write_position
        << std::endl;
      return 0; // TODO: LOG
    }

    size_t chunk_offset = write_position - chunk_index *M64; // offset % M64
    size_t chunk_size = std::min((uint64_t)(chunk_index+1) *M64, end) - write_position;

    Location & loc = chunk.location[0];
    ChunkServer *cs = Cached::Create(loc.ip, loc.port, false);
    if (!cs) {
      // just try again
      cs = Cached::Create(loc.ip, loc.port, false);
    }

    if (!cs) {
      // LOG
      return 0;
    }

    uint32_t writeid;
    ret = cs->WriteBlockInit(&chunk, &writeid);
    if (ret != STATUS_OK) {
      std::cerr << "  WriteBlockInit failed, ret:" << ret
        << std::endl;
      return 0;
    }

    uint64_t chunk_pos = chunk_offset;
    uint64_t blockno = chunk_offset >> 16;
    while (true) {
      uint32_t block_offset = chunk_pos - blockno *K64; // # offset % K64
      uint32_t block_size = std::min(uint64_t(chunk_offset + chunk_size), (blockno+1) *K64) - chunk_pos; 

      ret = cs->WriteBlock(&chunk, writeid, blockno, block_offset, block_size, (const uint8_t *)buf);

      if (ret != STATUS_OK) {
        // do not return, if error occurred
        delete cs;

        std::cerr << "  WriteBlock failed " << ret 
          << " blockno: " << blockno
          << " block_offset: " << block_offset
          << " block_size: " << block_size
          << std::endl;
        return 0;
      }

      chunk_pos += block_size;
      blockno ++;
      writeid ++;

      buf = (char *)buf + block_size;

      // block loop end check
      if (chunk_pos >= chunk_offset + chunk_size)
        break;
    }
    Cached::Return(cs, false);

    write_position += chunk_size;
    chunk_index ++;

    ret = master_->WriteChunkEnd(chunk.id, inode_, write_position);
    if (ret != STATUS_OK) {
      std::cerr << "WriteChunkEnd failed " << ret << std::endl;
      return 0;
    }

    // chunk loop end check
    if (write_position >= end)
      break;
  }
  position_ = write_position;
  return count;
}

uint32_t File::Read(char *buf, size_t count) {
  ASSERT(inode_);
  if (position_ >= MFS_MAX_FILE_SIZE)
    return EFBIG;
  
  uint64_t end = position_ + count;

  uint64_t write_position = position_;
  int chunk_index = write_position >> 26;
  while (true) {
    uint64_t file_length;
    Chunk chunk;
    int status = master_->ReadChunk(inode_, chunk_index, &file_length, &chunk);
    if (status != STATUS_OK) {
      std::cerr << "ReadChunk failed, status: " << status << "\n";
      return 0;
    }
    
    if (file_length < end)
      end = file_length;

    if (file_length < position_) {
      std::cerr << "position exceeded\n";
      return 0;
    }

    uint64_t chunk_offset = write_position - chunk_index * M64; // offset % M64
    uint64_t chunk_size = std::min(end, uint64_t((chunk_index+1) * M64)) - write_position;

    if (chunk.location.size() < 1) {
      std::cerr << "inode: " << inode_ << " location empty\n";
      time_t now = time(0);
       // hack for reconnected 
      if (now - last_registered_ < 10) {
        continue;
      }
      return 0; // TODO: log error
    }

    ChunkServer *cs = NULL;
    uint32_t goal_count = 0;
    while (!cs) {
      if (goal_count + 1 > chunk.location.size())
        break;
      Location loc = chunk.location[goal_count];
      cs = Cached::Create(loc.ip, loc.port, true);

      ++ goal_count;
    }

    if (!cs) {
      // LOG(WARNING) << 
      std::cerr << inode_ << " chunk server connect failed\n";
      return 0;
    }

    int ret = cs->ReadBlock(&chunk, chunk_offset, chunk_size, (uint8_t *)buf);

    if (ret != STATUS_OK) {
      // do not return, if error occurred
      delete cs;
      std::cerr << "chunk server ReadBlock failed ret: " << ret << "\n";

      time_t now = time(0);
      // hack for reconnected 
      if (now - last_registered_ < 10) {
        sleep(1);
        continue;
      }
    }
    Cached::Return(cs, true);

    // next chunk
    write_position += chunk_size;
    buf = (char *)buf + chunk_size;
    chunk_index ++;

    // # end condition
    if (write_position >= end)
      break;
  }

  uint32_t readed = write_position - position_;
  position_ = write_position;

  return readed;
}

uint64_t File::Seek(uint64_t offset, int whence) {
  uint64_t old_position = position_;
  FileAttribute attr;
  if (STATUS_OK == master_->GetAttr(inode_, &attr)) {
    if (whence == SEEK_SET && attr.size() >= offset) {
      position_ = offset;
      return old_position;
    }
    if (whence == SEEK_CUR && attr.size() >= offset + position_) {
      position_ += offset;
      return old_position;
    }
  }
  return -1;
}

uint32_t File::Pwrite(const char *buf, size_t count, off_t offset) {
  // TODO: very weak implement
  position_ = offset;
  return Write(buf, count);
}

uint32_t File::Pread(char *buf, size_t count, off_t offset) {
  // TODO: very weak implement
  position_ = offset;
  return Read(buf, count);
}

} // namespace moose
