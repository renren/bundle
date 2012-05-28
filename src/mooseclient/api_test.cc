#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h> // perror
#include <string.h> // memset

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "boost/static_assert.hpp"

#include "base3/ptime.h"
#include "base3/getopt_.h"
#include "mooseclient/sockutil.h"
#include "mooseclient/moose.h"

#ifndef ASSERT
#if defined(__GNUC__)
#define ASSERT(x) if(!(x)) {printf("ASSERT %s failed at line %d\n", #x, __LINE__); __asm__("int $3");}
#elif defined(_MSC_VER)
#define ASSERT(x) if(!(x)) __asm int 3;
#endif
#endif

#ifdef WIN32
  #include <winsock2.h>
  #define snprintf _snprintf
#endif

using namespace moose;

int DeleteForever(MasterServer *master, uint32_t parent, const char *name) {
  FileAttribute attr;
  uint32_t inode_it;
  int ret = master->Lookup(parent, name, &inode_it, &attr);
  if (STATUS_OK != ret)
    return ret;

  std::cerr << name << " type: " << (int)attr.type() << " ret: " << ret << " inode: " << inode_it << std::endl;  

  if (attr.type() == TYPE_DIRECTORY) {
    ret = master->Rmdir(parent, name);
    ASSERT(0 == ret);
  }
  else {
    ret = master->Unlink(parent, name);
    ASSERT(0 == ret);
  }
  
  return ret;
}

void TestOther(MasterServer *master) {
  // lookup, open, truncate, mkdir, rmdir, unlink, close
  // TODO: Mknod

  uint32_t inode_test;
  int ret = 0;

  // Mkdir: "/fooxxxx"
  {
    const char *prefix = "foo";
    uint32_t mode_arr[] = {0755, 0000, 0111, 0222, 0555, 0666, 0644};
    for (int i=0; i<sizeof(mode_arr)/sizeof(uint32_t); ++i) {
      char sz[32];
      snprintf(sz, 32, "%s%04o", prefix, mode_arr[i]);

      DeleteForever(master, MFS_ROOT_ID, sz);

      // # 
      FileAttribute attr;
      ret = master->Mkdir(MFS_ROOT_ID, sz, mode_arr[i], &inode_test, &attr);
      ASSERT(STATUS_OK == ret);

      ASSERT(attr.mode() == 040000 | mode_arr[i]);
      ASSERT(attr.type() == TYPE_DIRECTORY);

      {
        uint32_t inode_query;
        FileAttribute attr_query;
        master->Lookup(sz, &inode_query, &attr_query);
        ASSERT(inode_query == inode_test);
        ASSERT(0 == memcmp(attr.buf, attr_query.buf, 35));
      }

      // #
      ret = master->Rmdir(MFS_ROOT_ID, sz);
      ASSERT(STATUS_OK == ret);
    }
  }

  // Create | Mknod: "/barxxxx"
  {
    const char *prefix = "bar";
    uint32_t mode_arr[] = {0755, 0000, 0111, 0222, 0555, 0666, 0644};
    for (int i=0; i<sizeof(mode_arr)/sizeof(uint32_t); ++i) {
      char sz[32];
      snprintf(sz, 32, "%s%04o", prefix, mode_arr[i]);

      DeleteForever(master, MFS_ROOT_ID, sz);

      ret = master->Create(MFS_ROOT_ID, sz, mode_arr[i], &inode_test);
      ASSERT(STATUS_OK == ret);

      ret = master->Unlink(MFS_ROOT_ID, sz);
      ASSERT(STATUS_OK == ret);
    }
  }

  // Mkdir/Create /parent/childxxxxxxxx
  do {
    const char *parent = "parent";
    if (STATUS_OK == master->Lookup(MFS_ROOT_ID, parent, &inode_test, NULL)) {
      break;
      // ret = master->Rmdir(MFS_ROOT_ID, parent);
      // ASSERT(STATUS_OK == ret);
    }

    // #
    uint32_t inode_parent;
    FileAttribute attr;
    ret = master->Mkdir(MFS_ROOT_ID, parent, 0755, &inode_parent, &attr);
    ASSERT(STATUS_OK == ret);

    for (int i=0; i<1000; ++i) {
      char sz[32];
      snprintf(sz, 32, "child%08d", i);
      ret = master->Create(inode_parent, sz, 0644, &inode_test);
      ASSERT(STATUS_OK == ret);
    }

    ret = master->Rmdir(MFS_ROOT_ID, parent);
    ASSERT(ERROR_ENOTEMPTY == ret);
    ASSERT(STATUS_OK == master->Lookup(MFS_ROOT_ID, parent, &inode_test, NULL));
    ASSERT(inode_test == inode_parent);
  } while (false);

  // Create/Mkdir with same name
  {
    const char *name = "samename";

    ret = DeleteForever(master, MFS_ROOT_ID, name);
    std::cout << "DeleteForever " << name << " return " << ret << std::endl;

    ret = master->Create(MFS_ROOT_ID, name, 0755, &inode_test);
    ASSERT(STATUS_OK == ret);

    ret = master->Mkdir(MFS_ROOT_ID, name, 0755, &inode_test, NULL);
    ASSERT(ERROR_EEXIST == ret);

    DeleteForever(master, MFS_ROOT_ID, name);

    ret = master->Mkdir(MFS_ROOT_ID, name, 0755, &inode_test, NULL);
    ASSERT(STATUS_OK == ret);
    
    ret = master->Create(MFS_ROOT_ID, name, 0755, &inode_test);
    ASSERT(ERROR_EEXIST == ret);

    DeleteForever(master, MFS_ROOT_ID, name);
  }

  // Seek
  {
  }
}


int BUFFER_LENGTH = 64*1024;

void TestBenchRead(MasterServer *master, uint32_t inode, FileAttribute *attr
    , const char *normalfile, const char *mfsfile, int count) {
  char *buf = new char[BUFFER_LENGTH];

  FILE *fp = fopen(normalfile,"wb");
  if (!fp) {
    delete [] buf;
    return;
  }

  File fin(master);
  int ret = fin.Open(mfsfile, O_RDONLY, 0644);
  if (ret != STATUS_OK) {
    std::cerr << "open " << mfsfile << " error " << ret << std::endl;
    return;
  }

  while (count-- > 0) {
    fseek(fp, 0, SEEK_SET);

    base::ptime pt("read once");

    uint64_t pos = 0;
    size_t readed = 0;
    do {
      readed = fin.Read(buf, BUFFER_LENGTH);
      if (readed)
        fwrite(buf, 1, readed, fp);
      
      pos += readed;
    } while (readed == BUFFER_LENGTH);
  }
  fclose(fp);
  fin.Close();

  delete [] buf;
}

void TestBenchWrite(MasterServer *master, uint32_t inode, FileAttribute *attr
    , const char *normalfile, const char *mfsfile, int count) {
  char *buf = new char[BUFFER_LENGTH];

  FILE *fp = fopen(normalfile,"rb");
  if (!fp) {
    delete [] buf;
    return;
  }

  File fout(master);
  fout.Open(mfsfile, O_WRONLY | O_CREAT, 0644);

  while (count-- > 0) {
    fseek(fp, 0, SEEK_SET);

    base::ptime pt("write once");

    uint64_t pos = 0;
    size_t readed = 0;
    do {
      readed = fread(buf, 1, BUFFER_LENGTH, fp);
      if (readed) {
        uint64_t written = fout.Write(buf, readed);
        // std::cout << "Write " << written << std::endl;
        ASSERT(written == readed);

        pos += readed;
      }
    } while (readed == BUFFER_LENGTH);
  }
  fclose(fp);
  fout.Close();

  delete [] buf;
}

void Usage() {
  std::cerr << "Usage:  -r|w fromfile tofile \n"
    "\t-t other file operation test\n"
    "\t[-c bench test count] \n"
    "\t[-H master server] [-p master port]\n"
    "\t[-b buffer size(KB)]\n";
}

int main(int argc, char *argv[]) {
  // -H -p
  // -c count # bench count
  // -f 
  // -t
  std::string mfsfile, normalfile;
  std::string host = "127.0.0.1";
  int port = 9421;
  int benchcount = 1;
  int buffer_size = 0;

  enum {
    CMD_NONE, 
    CMD_READ, CMD_WRITE,
    CMD_OTHER
  } cmd = CMD_NONE;

  int opt;
  while ((opt = getopt(argc, argv, "rwthH:p:c:b:")) != -1) {
    switch (opt) {
    case 'H':
      host = optarg;
      break;
    case 'p':
      port = atoi(optarg);
    case 'b':
      buffer_size = atoi(optarg);
      break;
    case 'r':
      cmd = CMD_READ;
      break;
    case 'w':
      cmd = CMD_WRITE;
      break;
    case 't':
      cmd = CMD_OTHER;
      break;
    case 'c':
      benchcount = atoi(optarg);
      break;
    default:
      Usage();
      return 0;
    }
  }

  if (buffer_size) {
    BUFFER_LENGTH = buffer_size *1024;
  }

  if (cmd == CMD_NONE) {
    std::cerr << "test type missing, -r|-w|-t\n";
    Usage();
    return 1;
  }

  if (cmd != CMD_OTHER && optind >= argc - 1) {
    std::cerr << "test type missing, -r|-w|-t\n";
    Usage();
    return 1;
  }

  if (cmd == CMD_READ) {
    mfsfile = argv[optind];
    normalfile = argv[optind + 1];
  }
  else if (cmd == CMD_WRITE) {
    normalfile = argv[optind];
    mfsfile = argv[optind + 1];
  }
  
  BOOST_STATIC_ASSERT(sizeof(moose::FileAttribute) == 35);
#ifdef WIN32
  WSADATA wsa_data;
  WSAStartup(0x0201, &wsa_data);
#else
  // ignore SIGPIPE
  struct sigaction sa;
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGPIPE, &sa, (struct sigaction *) 0) < 0) {
    perror("signal");
    return -1;
  }

  if (SIG_ERR == signal(SIGPIPE, SIG_IGN)) {
    perror("signal");
    return -1;
  }
#endif

  std::string hostip;
  if (!host.empty()) {
    std::stringstream stem;
    stem << host << ":" << port;
    hostip = stem.str();
  }

  // 
  MasterServer master;
  if (!master.Connect(hostip.c_str())) {
    std::cerr << "master connect "
      << hostip
      << " failed\n";
    return -1;
  }

  FileAttribute attr;
  uint32_t inode_test;
#if 0
  {
    int ret = master.Lookup(mfsfile.c_str(), &inode_test, &attr);
    ASSERT(ret == STATUS_OK);
    if (ret != STATUS_OK) {
      std::cerr << "lookup " << mfsfile << " in mfs failed\n";
      return 1;
    }
  }
#endif

  if (cmd == CMD_OTHER) {
    TestOther(&master);
    return 0;
  }
  
  if (cmd == CMD_WRITE) {
    TestBenchWrite(&master, inode_test, &attr, normalfile.c_str(),
      mfsfile.c_str(), benchcount);
    return 0;
  }

  if (cmd == CMD_READ) {
    TestBenchRead(&master, inode_test, &attr, normalfile.c_str(), 
      mfsfile.c_str(), benchcount);
    return 0;
  }

  return 0;
}
