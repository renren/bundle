#include <signal.h>

#include <stdlib.h>
#include <stdio.h> // perror
#include <string.h> // memset

#include "boost/static_assert.hpp"

#include "moose/moose.h"


#ifndef WIN32
#include "mfscommon/datapack.h"
#else
#include "mfscommon/datapack_win.h"
#endif

#include "moose/sockutil.h"

#ifndef ASSERT
#if defined(__GNUC__)
#define ASSERT(x) if(!(x)) {printf("ASSERT failed %s %d\n", #x, __LINE__); __asm__("int $3");}
#elif defined(_MSC_VER)
  #define ASSERT(x) if(!(x)) __asm int 3;
#endif
#endif

#ifndef evutil_timersub
#define	evutil_timersub(tvp, uvp, vvp)					\
do {								\
  (vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;		\
  (vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;	\
  if ((vvp)->tv_usec < 0) {				\
  (vvp)->tv_sec--;				\
  (vvp)->tv_usec += 1000000;			\
  }							\
} while (0)
#endif

int count_read = 0;
struct timeval tv_start;

void count_once() {
  count_read ++;

  if (count_read % 10000 == 0) {
    struct timeval now, diff;
    gettimeofday(&now, NULL);
    evutil_timersub(&now, &tv_start, &diff);

    // usec = total_time.tv_sec * 1000000 + total_time.tv_usec;
    if (diff.tv_sec)
      printf("%d %f\n", count_read, double(count_read) / diff.tv_sec);
  }
}

struct Buffer {
  uint8_t * ptr;
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

void FileAttribute::to_stat(uint32_t inode, struct stat * stbuf) const {
  mfs_attr_to_stat(inode, buf, stbuf);
}


struct Chunk {
  uint64_t id;
  // struct Location
};

class ChunkServer {
public:
  bool Connect(struct sockaddr* addr, socklen_t addr_len) {
    return true;
  }
private:
};

class Master {
public:
  Master() : fd_(0), id_(0)
  , session_id_(0)
  , uid_(0), gid_(0)
  , auto_reconnect_(true)
  , hostaddr_(0)
  , hostaddr_len_(0)
  {}

  ~Master() {
    if (fd_)
      close(fd_);
    delete [] hostaddr_;
  }

  // synchronized connect, and register
  bool Connect(struct sockaddr* addr, socklen_t addr_len) {
    ASSERT(0 == fd_);
    fd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ASSERT(fd_ > 0);
    if (fd_ < 0) {
      perror("socket");
      return false;
    }

    int ret = connect(fd_, addr, addr_len);
    ASSERT(ret == 0);
    if (ret < 0) {
      perror("connect");
      
      close(fd_);
      fd_ = 0;
      
      return false;
    }

    Buffer buf;
    ret = BuildRegister(&buf);
    if (!ret)
      return false;

    ret = write(fd_, buf.ptr, buf.used);
    ASSERT(ret == buf.used);
    if (-1 == ret) {
      perror("write");
      return false;
    }

    int cmd, length;
    ret = ReadCommand(&buf, &cmd, &length);
    // printf("read ref %d %d\n", cmd, length);
    ASSERT(ret);
    ASSERT(cmd == MATOCU_FUSE_REGISTER);
    
    // TODO: avoid magic 8
    ret = GotRegister(cmd, buf.ptr + 8, length);
    ASSERT(session_id_);

    if (auto_reconnect_) {
      hostaddr_ = new char[addr_len];
      memcpy(hostaddr_, addr, addr_len);
      hostaddr_len_ = addr_len;
    }
    return true;
  }

  void EnsureConnect() {
    if (fd_)
      return;

    if (!hostaddr_)
      return;

    if (auto_reconnect_ && hostaddr_) {
      // hack little dirty
      auto_reconnect_ = false;
      Connect((struct sockaddr*)hostaddr_, hostaddr_len_);
      auto_reconnect_ = true;
    }
  }

  enum {
    VERSMAJ = 1,
    VERSMID = 1,
    VERSMIN = 1,
  };

  bool BuildRegister(struct Buffer * output) {
    const char info[] = "/libmoose/mount";
    const char subfolder[] = "/";

    int ileng = sizeof(info);
    int pleng = sizeof(subfolder);

    const int message_length = 8 + 64 + 13 + ileng + pleng;

    if (!output->Alloca(message_length)) {
      // TODO: LOG error
      return false;
    }

    uint8_t * wptr = output->ptr;

    put32bit(&wptr, CUTOMA_FUSE_REGISTER);

    // TODO: password
    put32bit(&wptr, 64 + 13 + ileng + pleng);
    memcpy(wptr, FUSE_REGISTER_BLOB_ACL, 64);
    wptr+=64;

    if (0 == session_id_) {
      // TODO: REGISTER_NEWMETASESSION
      put8bit(&wptr,REGISTER_NEWSESSION);
    } else {
      put8bit(&wptr,REGISTER_RECONNECT);
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

  bool GotRegister(uint32_t cmd, const uint8_t* payload, int length) {
    // TDOO: session_flags, uid, gid, map uid, map gid
    uint32_t sid = get32bit(&payload);
    if (session_id_ == 0)
      session_id_ = sid;
    else
      ASSERT(sid == session_id_);
    return true;
  }

  // TODO:
  bool QueryInfo(Buffer * output) {
    const int message_length = 8;
    if (!output->Alloca(message_length)) {
      return false;
    }

    uint8_t * wptr = output->ptr;
    put32bit(&wptr, CUTOMA_INFO);
    put32bit(&wptr, 0);
    output->used = wptr - output->ptr;

    return true;
  }

  int Lookup(uint32_t parent, const char * name, uint32_t * inode, FileAttribute * attr) {
    EnsureConnect();

    int nleng = strlen(name);

    Buffer buf;
    uint8_t * wptr = BuildCommand(&buf, CUTOMA_FUSE_LOOKUP, 13 + nleng);

    put32bit(&wptr,parent);
    put8bit(&wptr,nleng);
    memcpy(wptr,name,nleng);
    wptr+=nleng;
    put32bit(&wptr,uid_);
    put32bit(&wptr,gid_);

    buf.used = wptr - buf.ptr;

    int ret = write(fd_, buf.ptr, buf.used);
    ASSERT(ret == buf.used);

    int cmd, length;
    ret = ReadCommand(&buf, &cmd, &length);
    ASSERT(ret);
    ASSERT(cmd == MATOCU_FUSE_LOOKUP);

    const uint8_t * rptr = buf.ptr + 8;
    uint32_t packet_id = get32bit(&rptr);
    ASSERT(packet_id == session_id_);
    if (length == 4 + 5) {
      uint8_t status = get8bit(&rptr);
      printf("lookup %s failed %d\n", name, status);
      return status;
    }

    if (length == 4 + 39) {
      uint32_t t = get32bit(&rptr);
      if (inode)
        *inode = t;
      if (attr)
        memcpy(attr, rptr, 35);
      return STATUS_OK;
    }

    // TODO: remove
    printf("lookup %s unexpected size %d\n", name, length);
    // TODO: disconnect
    return ERROR_IO;
  }

  // WANT_READ | WANT_WRITE | WANT_CREATE
  bool Open(uint32_t inode, uint8_t flags, FileAttribute * attr) {
    EnsureConnect();

    Buffer buf;
    uint8_t * wptr = BuildCommand(&buf, CUTOMA_FUSE_OPEN, 13);
    put32bit(&wptr,inode);
    put32bit(&wptr,uid_);
    put32bit(&wptr,gid_);
    put8bit(&wptr,flags);

    buf.used = wptr - buf.ptr;

    int ret = write(fd_, buf.ptr, buf.used);
    ASSERT(ret == buf.used);

    int cmd, length;
    ret = ReadCommand(&buf, &cmd, &length);
    ASSERT(ret);
    ASSERT(cmd == MATOCU_FUSE_OPEN);

    const uint8_t * rptr = buf.ptr + 8;
    uint32_t packet_id = get32bit(&rptr);
    ASSERT(packet_id == session_id_);
    if (length == 4 + 1)
      return rptr[0] == STATUS_OK;

    if (length == 4 + 35 && attr) {
      memcpy(attr, rptr, 35);
      return true;
    }

    // TODO: disconnect
    return false;
  }

  int Mknod(uint32_t parent, const char* name, uint8_t type, uint16_t mode, uint32_t rdev, uint32_t * inode, FileAttribute * attr) {
    EnsureConnect();

    int nleng = strlen(name);

    Buffer buf;
    uint8_t * wptr = BuildCommand(&buf, CUTOMA_FUSE_MKNOD, 20 + nleng);
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

    int ret = write(fd_, buf.ptr, buf.used);
    ASSERT(ret == buf.used);

    int cmd, length;
    ret = ReadCommand(&buf, &cmd, &length);
    ASSERT(ret);
    ASSERT(cmd == MATOCU_FUSE_MKNOD);

    const uint8_t * rptr = buf.ptr + 8;
    uint32_t packet_id = get32bit(&rptr);
    ASSERT(packet_id == session_id_);
    if (length == 4 + 1) {
      // TODO: disconnect
      return rptr[0];
    }

    if (length != 4 + 39) {
      return ERROR_IO;
    }

    uint32_t t = get32bit(&rptr);
    if (inode)
      *inode = t;
    // TODO: cache it
    if (attr)
      memcpy(attr, rptr, 35);
    return STATUS_OK;
  }

  bool Mkdir(uint32_t parent, const char* name, uint16_t mode, uint32_t * inode, FileAttribute * attr) {
    EnsureConnect();

    int nleng = strlen(name);

    Buffer buf;
    uint8_t * wptr = BuildCommand(&buf, CUTOMA_FUSE_MKDIR, 15 + nleng);
    put32bit(&wptr,parent);
    put8bit(&wptr,nleng);
    memcpy(wptr,name,nleng);
    wptr+=nleng;
    put16bit(&wptr,mode);
    put32bit(&wptr,uid_);
    put32bit(&wptr,gid_);

    buf.used = wptr - buf.ptr;

    int ret = write(fd_, buf.ptr, buf.used);
    ASSERT(ret == buf.used);

    int cmd, length;
    ret = ReadCommand(&buf, &cmd, &length);
    ASSERT(ret);
    ASSERT(cmd == MATOCU_FUSE_MKDIR);

    const uint8_t * rptr = buf.ptr + 8;
    uint32_t packet_id = get32bit(&rptr);
    ASSERT(packet_id == session_id_);
    if (length == 4 + 1)
      return rptr[0] == STATUS_OK;

    if (length != 4 + 39) {
      // TODO: disconnect
      return false;
    }

    uint32_t t = get32bit(&rptr);
    if (inode)
      *inode = t;
    // TODO: cache it
    if (attr)
      memcpy(attr, rptr, 35);
    return STATUS_OK;
  }

  int Rmdir(uint32_t parent, const char* name) {
    EnsureConnect();

    int nleng = strlen(name);

    Buffer buf;
    uint8_t * wptr = BuildCommand(&buf, CUTOMA_FUSE_RMDIR, 13 + nleng);
    put32bit(&wptr,parent);
    put8bit(&wptr,nleng);
    memcpy(wptr,name,nleng);
    wptr+=nleng;
    put32bit(&wptr,uid_);
    put32bit(&wptr,gid_);

    buf.used = wptr - buf.ptr;

    int ret = write(fd_, buf.ptr, buf.used);
    ASSERT(ret == buf.used);

    int cmd, length;
    ret = ReadCommand(&buf, &cmd, &length);
    ASSERT(ret);
    ASSERT(cmd == MATOCU_FUSE_RMDIR);

    const uint8_t * rptr = buf.ptr + 8;
    uint32_t packet_id = get32bit(&rptr);
    ASSERT(packet_id == session_id_);
    if (length == 4 + 1)
      return rptr[0];

    // TODO: disconnect
    return ERROR_IO;
  }

  int Unlink(uint32_t parent, const char* name) {
    EnsureConnect();

    int nleng = strlen(name);

    Buffer buf;
    uint8_t * wptr = BuildCommand(&buf, CUTOMA_FUSE_UNLINK, 13 + nleng);
    put32bit(&wptr,parent);
    put8bit(&wptr,nleng);
    memcpy(wptr,name,nleng);
    wptr+=nleng;
    put32bit(&wptr,uid_);
    put32bit(&wptr,gid_);

    buf.used = wptr - buf.ptr;

    int ret = write(fd_, buf.ptr, buf.used);
    ASSERT(ret == buf.used);

    int cmd, length;
    ret = ReadCommand(&buf, &cmd, &length);
    ASSERT(ret);
    ASSERT(cmd == MATOCU_FUSE_UNLINK);

    const uint8_t * rptr = buf.ptr + 8;
    uint32_t packet_id = get32bit(&rptr);
    ASSERT(packet_id == session_id_);
    if (length == 4 + 1)
      return rptr[0];

    // TODO: disconnect
    return ERROR_IO;
  }

  int Create(uint32_t parent, const char* name, mode_t mode, uint32_t * inode) {
    return ERROR_IO;
  }

  int Close(uint32_t inode) {
    return ERROR_IO;
  }

  size_t Read(uint32_t inode, size_t size, size_t offset, void* buf) {
    return 0;
  }

  size_t Write(uint32_t inode, const char* buf, size_t size, size_t offset) {
    return 0;
  }

  bool ReadCommand(struct Buffer *input, int * cmd_, int * length_) {
    input->Alloca(8);
    
    int ret = read(fd_, input->ptr, 8);
    if (ret < 0) {
      perror("read command");
      return false;
    }
    // printf("read 1st %d\n", ret);
    if (8 != ret) {
      return false;
    }
    
    const uint8_t * mem = input->ptr;

    uint32_t cmd = get32bit(&mem);
    uint32_t length = get32bit(&mem);
    // printf("got cmd %d length %d\n", cmd, length);

    if (length > 64 * 1024 *1024) {
      // TODO: log error
      return false;
    }

    if (cmd_)
      *cmd_ = cmd;

    if (length_)
      *length_ = length;

    input->Alloca(8 + length);

    ret = read(fd_, input->ptr + 8, length);
    // printf("read 2nd %d\n", ret);
    if (ret == length)
      return true;
    return false;
  }

  uint8_t * BuildCommand(struct Buffer *output, uint32_t cmd, uint32_t length) {
    output->Alloca(8 + 4 + length);

    uint8_t * wptr = output->ptr;
    put32bit(&wptr, cmd);
    put32bit(&wptr, 4 + length);
    put32bit(&wptr, session_id_);

    return wptr;
  }

#if 0
  void EventCallback(struct bufferevent *bev, short events) {
    if (events & BEV_EVENT_CONNECTED) {
      Buffer *output = bufferevent_get_output(bev);
      BuildRegister(output);
      // QueryInfo(output);
    }
  }

  
  bool GotCommand(uint32_t cmd, const uint8_t* payload, int length) {
    printf("with payload cmd: %d length: %d\n", cmd, length);
    typedef bool (Master::*Handle)(uint32_t, const uint8_t*, int);
    struct HandlePair { 
      uint32_t cmd;
      Handle handle;
    };
    struct HandlePair table[] = {
      {ANTOAN_NOP, &Master::GotPing},
      {MATOCU_FUSE_REGISTER, &Master::GotRegister},
      {-1, NULL},
    };

    for (HandlePair* p = table; ; p++) {
      if (!p->handle) {
        // TODO: LOG
        return false;
      }

      if (cmd != p->cmd)
        continue;

      bool deal = (this->*(p->handle))(cmd, payload, length);
      if (deal)
        break;
    }
    return true;
  }

  bool GotPing(uint32_t cmd, const uint8_t* payload, int length) {
    // TDOO: ping back
    return true;
  }

  

  // shit below
  static void _EventCallback(struct bufferevent *bev, short events, void *ctx) {
    printf("event callback %x\n", events);
    Master* self = (Master*)ctx;
    if (self)
      self->EventCallback(bev, events);
  }

  static void _ReadCallback(struct bufferevent *bev, void *ctx) {
    Buffer *input = bufferevent_get_input(bev);
    int length = evbuffer_get_length(input);
    printf("read %d\n", length);

    Master* self = (Master*)ctx;
    self->ReadCallback(bev);
    return;

    // http://www.wangafu.net/~nickm/libevent-book/Ref7_evbuffer.html
    // evbuffer_drain
    // evbuffer_remove

    
    void* buf = malloc(length + 1);
    evbuffer_remove(input, buf, length);
    ((char*)buf)[length] = '\0';

    // printf("read [%d] %s\n", length, (const char*)buf);
    // printf("%d ", length);
    free(buf);

    count_once();

    Buffer *output = bufferevent_get_output(bev);
    evbuffer_add_printf(output, "hello %d", self->id_++);
  }

  static void _WriteCallback(struct bufferevent *bev, void *ctx) {
    Buffer *output = bufferevent_get_output(bev);
    int length = evbuffer_get_length(output);
    printf("write %d\n", length);

    Master* self = (Master*)ctx;
  }
#endif

  int session_id() const {
    return session_id_;
  }

private:
  int fd_;

  int id_;
  uint32_t session_id_;
  int uid_, gid_;
  bool auto_reconnect_;
  char * hostaddr_;
  int hostaddr_len_;
};

void Test() {
  Master master;

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  int outlen = sizeof(addr);
  parse_sockaddr_port("10.22.206.244:9421", (struct sockaddr*)&addr, &outlen);

  if (!master.Connect((struct sockaddr*)&addr, outlen)) {
    printf("master connect failed\n");
    return;
  }

  printf("registered session:%d\n", master.session_id());

  uint32_t inode_test;
  {
    FileAttribute attr;
    int ret = master.Lookup(MFS_ROOT_ID, "test", &inode_test, &attr);
    ASSERT(ret == STATUS_OK);
    printf("%d \ttest %c %o\n", inode_test, attr.type(), attr.mode());
  }

  {
    FileAttribute attr;
    uint32_t inode_to_remove;
    int ret = master.Mkdir(inode_test, "_to_remove", 0755, &inode_to_remove, &attr);
    ASSERT(ret == STATUS_OK);
    printf("%d \t_to_remove %c %o\n", inode_to_remove, attr.type(), attr.mode());
  }

  if (1) {
    int ret = master.Rmdir(inode_test, "_to_remove");
    ASSERT(ret == STATUS_OK);
    printf("rmdir %d\n", ret);
  }

  {
    FileAttribute attr;
    uint32_t inode_to_remove;
    int ret = master.Mknod(inode_test, "_to_remove_file", TYPE_FILE, 0644, 0
      , &inode_to_remove, &attr);
    ASSERT(ret == STATUS_OK);
    printf("%d \t_to_remove_file %c 0%o\n", inode_to_remove, attr.type(), attr.mode());

    // Open
    bool f = master.Open(inode_to_remove, WANT_WRITE, &attr);
    ASSERT(f);
    printf("open %d %c 0%o\n", inode_to_remove, attr.type(), attr.mode());
  }

  if (1) {
    int ret = master.Unlink(inode_test, "_to_remove_file");
    ASSERT(ret == STATUS_OK);
    printf("unlink %d\n", ret);
  }

  {
    FileAttribute attr;
    uint32_t inode_jpg;
    int ret = master.Lookup(MFS_ROOT_ID, "sample.jpg", &inode_jpg, &attr);
    ASSERT(ret == STATUS_OK);

    char * buf = new char[attr.size()];
    size_t readed = master.Read(inode_jpg, attr.size(), 0, buf);
    ASSERT(readed == attr.size());
    delete [] buf;
  }

  getchar();

  

  // open
  // master.Create(
  // read

  getchar();

  // master.QueryInfo();
}

int main(int argc, char* argv[]) {
  BOOST_STATIC_ASSERT(sizeof(FileAttribute) == 35);
#ifdef WIN32
  WSADATA wsa_data;
  WSAStartup(0x0201, &wsa_data);

#else

  // ignore SIGTRAP
  struct sigaction sa;
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGTRAP, &sa, (struct sigaction *) 0) < 0) {
    perror("signal");
    return -1;
  }

#endif

  if (SIG_ERR == signal(SIGTRAP, SIG_IGN)) {
    perror("signal");
    return -1;
  }

  gettimeofday(&tv_start, NULL);

  Test();

  return 0;
}
