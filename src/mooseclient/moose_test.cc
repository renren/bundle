#include "gtest/gtest.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "boost/static_assert.hpp"
#include "base3/logging.h"
#include "base3/pathops.h"

#include "mooseclient/moose.h"

#if defined(OS_WIN)
  #include <winsock2.h>
  #define snprintf _snprintf
#endif

using namespace moose;

static std::string hostip_;

class WithMoose : public testing::Test {
protected:
  void SetUp() {
    master_.Connect(hostip_.c_str());
  }

  void TearDown() {
    master_.Disconnect();
  }

  MasterServer master_;
};

std::string FakeString(int size = 100, char ch = 'a') {
  std::string content(size, ch);
  for (int j=39; j<content.size(); j+=40)
    content[j] = '\n';

  content[content.size() - 1] = '\n';
  return content;
}

int extract(MasterServer *master_, const char *pathname, uint32_t *parent_inode
  , std::string *basename) {
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

int DeleteAny(MasterServer *master, const char *name) {
  uint32_t parent_inode;
  std::string basename;
  extract(master, name, &parent_inode, &basename);
  return master->Unlink(parent_inode, basename.c_str());
}

TEST_F(WithMoose, WriteAndRead) {
  const int M = 1024 * 1024;
  int arr[] = {
    1, 10, 100, 1023, 1024, 1025 // 0 - 5
    , 1024*4, 1024*4+1, 
    1024*1024,
    1024*1024 + 1,
    1024*1024 + 100,  // 10
    M * 2, M * 2 + 1, M * 3 -1, M * 4, M * 5,    // k l m n o
    M * 6, M * 7 + 1, M * 8 -1, M * 9, M * 10   // p q r s t
    // 1024*1024*64-1, 1024*1024*64, 1024*1024*64+1
  };

  for (int i=0; i<sizeof(arr)/sizeof(*arr); ++i) {
    std::string content = FakeString(arr[i], 'a' + i%26);

    char fn[260];
    snprintf(fn, sizeof(fn), "test/foo%d", i);;
    DeleteAny(&master_, fn);

    std::cout << "size: " << arr[i] << "\n";

    {
      File fout(&master_);
      int ret = fout.Open(fn, O_WRONLY | O_CREAT, 0644);
      ASSERT_EQ(0, ret);

      ret = fout.Write(content.data(), content.size());
      EXPECT_EQ(content.size(), ret);

      ASSERT_EQ(content.size(), fout.Tell());

      ret = fout.Close();
      ASSERT_EQ(0, ret);
    }

    {
      File fin(&master_);
      int ret = fin.Open(fn, O_RDONLY, 0644);

      char * buf = new char[content.size() + 1];
      ret = fin.Read(buf, content.size());
      ASSERT_EQ(content.size(), ret);
      buf[content.size()] = 0;

      // std::cout << buf;
      EXPECT_TRUE(content == buf) << " size:" << arr[i];

      ret = fin.Close();
      ASSERT_EQ(0, ret);
    }
  }
}

int DeleteForever(MasterServer *master, uint32_t parent, const char *name) {
  FileAttribute attr;
  uint32_t inode_it;
  int ret = master->Lookup(parent, name, &inode_it, &attr);
  if (STATUS_OK != ret)
    return ret;

  if (attr.type() == TYPE_DIRECTORY) {
    ret = master->Rmdir(parent, name);
    LOG_ASSERT(0 == ret);
  }
  else {
    ret = master->Unlink(parent, name);
    LOG_ASSERT(0 == ret);
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
      LOG_ASSERT(STATUS_OK == ret);

      LOG_ASSERT(attr.mode() == 040000 | mode_arr[i]);
      LOG_ASSERT(attr.type() == TYPE_DIRECTORY);

      {
        uint32_t inode_query;
        FileAttribute attr_query;
        master->Lookup(sz, &inode_query, &attr_query);
        LOG_ASSERT(inode_query == inode_test);
        LOG_ASSERT(0 == memcmp(attr.buf, attr_query.buf, 35));
      }

      // #
      ret = master->Rmdir(MFS_ROOT_ID, sz);
      LOG_ASSERT(STATUS_OK == ret);
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
      LOG_ASSERT(STATUS_OK == ret);

      ret = master->Unlink(MFS_ROOT_ID, sz);
      LOG_ASSERT(STATUS_OK == ret);
    }
  }

  // Mkdir/Create /parent/childxxxxxxxx
  do {
    const char *parent = "parent";
    if (STATUS_OK == master->Lookup(MFS_ROOT_ID, parent, &inode_test, NULL)) {
      break;
      // ret = master->Rmdir(MFS_ROOT_ID, parent);
      // LOG_ASSERT(STATUS_OK == ret);
    }

    // #
    uint32_t inode_parent;
    FileAttribute attr;
    ret = master->Mkdir(MFS_ROOT_ID, parent, 0755, &inode_parent, &attr);
    LOG_ASSERT(STATUS_OK == ret);

    for (int i=0; i<1000; ++i) {
      char sz[32];
      snprintf(sz, 32, "child%08d", i);
      ret = master->Create(inode_parent, sz, 0644, &inode_test);
      LOG_ASSERT(STATUS_OK == ret);
    }

    ret = master->Rmdir(MFS_ROOT_ID, parent);
    LOG_ASSERT(ERROR_ENOTEMPTY == ret);
    LOG_ASSERT(STATUS_OK == master->Lookup(MFS_ROOT_ID, parent, &inode_test, NULL));
    LOG_ASSERT(inode_test == inode_parent);
  } while (false);

  // Create/Mkdir with same name
  {
    const char *name = "samename";

    ret = DeleteForever(master, MFS_ROOT_ID, name);
    std::cout << "DeleteForever " << name << " return " << ret << std::endl;

    ret = master->Create(MFS_ROOT_ID, name, 0755, &inode_test);
    LOG_ASSERT(STATUS_OK == ret);

    ret = master->Mkdir(MFS_ROOT_ID, name, 0755, &inode_test, NULL);
    LOG_ASSERT(ERROR_EEXIST == ret);

    DeleteForever(master, MFS_ROOT_ID, name);

    ret = master->Mkdir(MFS_ROOT_ID, name, 0755, &inode_test, NULL);
    LOG_ASSERT(STATUS_OK == ret);
    
    ret = master->Create(MFS_ROOT_ID, name, 0755, &inode_test);
    LOG_ASSERT(ERROR_EEXIST == ret);

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

    uint64_t pos = 0;
    size_t readed = 0;
    do {
      readed = fread(buf, 1, BUFFER_LENGTH, fp);
      if (readed) {
        uint64_t written = fout.Write(buf, readed);
        // std::cout << "Write " << written << std::endl;
        LOG_ASSERT(written == readed);

        pos += readed;
      }
    } while (readed == BUFFER_LENGTH);
  }
  fclose(fp);
  fout.Close();

  delete [] buf;
}

void Usage() {
  std::cerr << "Usage: -H 127.0.0.1:9421 --gtest_filter=\n";
}

int main(int argc, char *argv[]) {
  for (int i=1; i<argc-1; ++i) {
    if (std::string(argv[i]) == "-H") {
      hostip_ = argv[i+1];
      break;
    }
  }

  if (hostip_.empty()) {
    Usage();
    return 1;
  }

  BOOST_STATIC_ASSERT(sizeof(moose::FileAttribute) == 35);

#ifdef OS_WIN
  WSADATA wsa_data;
  WSAStartup(0x0201, &wsa_data);
#elif defined(OS_POSIX)
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

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
