#include <stdio.h>
#include <limits>
#include <stdarg.h>
#include <errno.h>

#ifdef OS_WIN

#define popen _popen
#define pclose _pclose
#define ssize_t int64_t
#endif

#include <string>
#include <vector>

#include <boost/scoped_array.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include "gtest/gtest.h"

#include "bundle/bundle.h"
#include "base3/pathops.h"
#include "bundle/filelock.h"

struct foo {
  bundle::Info info;
  const char *correct_url;
};

void RestoreSetting() {
  bundle::Setting default_setting = {
    bundle::kMaxBundleSize,
    bundle::kBundleCountPerDay,
    50,
    400,
    &bundle::ExtractSimple, &bundle::BuildSimple
  };
  bundle::SetSetting(default_setting);
}


TEST(Bundle, URL) {
  foo arr[] = {
    {bundle::Info("a", 1, 100, 100, ".jpg"), "a/B/Bq/Bq/EV7kdt.jpg"},
#if 0
    {"a", 2, 100, 100, ".jpg", "a/C/Bq/Bq/CR33M9.jpg"},
    {"a/b", 1024, 100, 100, ".jpg", "a/b/SE/Bq/Bq/Xtc8w.jpg"},
    {"a/b/c", 20000, 100, 100, ".jpg", "a/b/c/FjV/Bq/Bq/CpF9ow.jpg"},
    {"a/b/c", 3, 0, 0, ".jpg", "a/b/c/D/A/A/BbKKbQ.jpg"},
    {"a/b/c", 16, 0, 1, ".jpg", "a/b/c/R/A/B/EmeovP.jpg"},
    {"a/b/c", 3, 0, 12983712, ".jpg", "a/b/c/D/A/BAGlN/EXE2Zz.jpg"},
    {"a/b/c", 3, 2348924, 12983712, ".jpg", "a/b/c/D/L2du/BAGlN/EcgZrK.jpg"},

		{"n", 0, 0, 0, ".jpg", "n/A/A/A/oaK13.jpg"},
		{"n", 0, 100, 100, ".jpg", "n/A/Bq/Bq/D3oYBN.jpg"}, // #10

    // 2G test
		{"n", 2u*1024*1024*1024, 2u*1024*1024*1024, 100, ".jpg", 0},
		{"n", 0, 2u*1024*1024*1024, 100, ".jpg", 0},
		{"n", 0, 2u*1024*1024*1024, 2u*1024*1024*1024, ".jpg", 0},
		{"n", 0, std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max(), ".jpg", 0},
		{"n", 0, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max(), ".jpg", 0},
#endif
  };

  for (int i=0; i<sizeof(arr)/sizeof(*arr); ++i) {
    foo *f = &arr[i];
    std::string url = bundle::BuildSimple(f->info);

#if 0
		std::cout << url << "\n";
#endif

    
    bundle::Info info_extracted;
    
    bool flag = bundle::ExtractSimple(url.c_str(), &info_extracted);
		EXPECT_TRUE(flag) << "  > url: " << url << " #" << i;

    EXPECT_EQ(f->info.id, info_extracted.id);
    // EXPECT_TRUE(f->info == info_extracted);
  }
}

TEST(Bundle, falseurl) {
	// incorrect url test
	// correct is: a/C/Bq/Bq/CR33M9.jpg
	const char *us[] = {
		"a/D/Bq/Bq/CR33M9.jpg",
		"a/D/Br/Cq/CR33M8.jpg",
		"a/C/Bq/Bq/CR33M9.a",
		"a/C/Bq/Bq/CR33M9.",
		"a/C/Bq/Bq/CR33M9.jpga",
		"a/D/Br/Cq/CR33M8.jpeg",
		"b/C/Bq/Bq/CR33M9.jpg",
		"1/C/Bq/Bq/CR33M9.jpg",
		"a/2/Bq/Bq/CR33M9.jpg",
		"a/C/Bq/00/CR33M9.jpg",
		"a/C/Bq/BQ/CR33M9.jpg",
		"a/C/Bq",
		"",
		"a/",
	};


	for (int i=0; i<sizeof(us)/sizeof(*us); ++i) {
		EXPECT_FALSE(bundle::ExtractSimple(us[i], NULL));
	}
}

TEST(Bundle, Example) {
  const char *storage = "test";
  const char *lockdir = "examplelock";

  // write
  const char *content = "content of file";
  const int length = strlen(content);

  bundle::Writer *writer = bundle::Writer::Allocate("p/20120512", ".txt"
      , length, storage);
	ASSERT_TRUE(0 != writer);

  size_t written;
  int ret = writer->Write(content, length, &written);
  ASSERT_EQ(0, ret) << "return ret:" << ret;

  std::string url = writer->EnsureUrl();
  std::cout << "write success, url: " << url << std::endl;

  delete writer;

  // read
#if 0
  std::string buf;
  ret = bundle::Reader::Read(url, &buf, storage);
  ASSERT_EQ(0, ret);
  ASSERT_EQ(buf, content);
#endif
}

std::string Execute(const char *prog, ...) {
	va_list arg;
	const char *s;
	std::string t(prog);

	va_start(arg, prog);
	// for (int i=0; (s = va_arg(arg, const char *)) !=0; i++) {
	while((s = va_arg(arg, const char *)) != 0) {
		t.append(s);
	}
	va_end(arg);

	EXPECT_TRUE(!t.empty()) << "command should not be empty";

	std::string output;
	FILE *fp = popen(t.c_str(), "r");
	EXPECT_TRUE(fp != 0) << "command is " << t;
	if (!fp)
		return output;

	while (!feof(fp)) {
		char buffer[100];
		int count = fread(buffer, sizeof( char ), sizeof(buffer), fp);
		if (count) {
			output.append(buffer, count);
		}
	}
#if 0
	ssize_t readed = 0;
	size_t len = 0;
	char *line = 0;
	while ((readed = getline(&line, &len, fp)) != -1) {
		printf("Retrieved line of length %u :\n", readed);
		printf(">>%s<<", line);
		if (len)
			output += std::string(line, len);
	}
	free(line);
#endif

	pclose(fp);
	return output;
}

TEST(Execute, Test) {
	EXPECT_EQ("a\n", Execute("echo a", 0));

	EXPECT_EQ("", Execute("touch foo", 0));
	EXPECT_EQ("1\n", Execute("find . -name foo | wc -l", 0));
}

TEST(Bundle, AllocateReuse) {
  const char *storage = "test";
  const char *prefix = "donot-ls-here";
  const char *postfix = ".txt";
  const char *lock_dir = ".lock";

  const char *content = "content of file";
  const int length = strlen(content);

	// empty mock/ first
  Execute("rm -rf ", storage, 0);
  Execute("rm -rf ", lock_dir, 0);

	// allocate once, make sure only one bundle created
	{
		bundle::Writer *writer = bundle::Writer::Allocate(prefix, postfix
			, length, storage, lock_dir);
		EXPECT_TRUE(writer != NULL);

    delete writer;

#if !defined(USE_MOOSECLIENT)
		EXPECT_EQ("1\n", Execute("find ", storage, " -type f | wc -l", 0));

    EXPECT_EQ("0\n", Execute("find ", lock_dir, " -type f | wc -l", 0));
#endif
	}

	// allocate again, without write, make sure no more bundle created
	{
		bundle::Writer *writer = bundle::Writer::Allocate(prefix, postfix
			, length, storage, lock_dir);
		EXPECT_TRUE(writer != NULL);

    delete writer;

#if !defined(USE_MOOSECLIENT)
		EXPECT_EQ("1\n", Execute("find ", storage, " -type f | wc -l", 0));

    EXPECT_EQ("0\n", Execute("find ", lock_dir, " -type f | wc -l", 0));
#endif
	}

	// exceed the limit count 100
	bundle::Setting a_limited_setting = {
		bundle::kBundleHeaderSize + bundle::kFileHeaderSize + length
			, 100, 10,10, &bundle::ExtractSimple, &bundle::BuildSimple};
	bundle::SetSetting(a_limited_setting);

  for (int i=0; i<600; i++) {
    //printf("---number : %d\n", i);
    bundle::Writer *writer = bundle::Writer::Allocate(prefix, postfix
        , length, storage, lock_dir);
    ASSERT_TRUE(NULL != writer);

		int ret = writer->Write(content,length);
		ASSERT_EQ(0, ret);

    delete writer;
  }

#if !defined(USE_MOOSECLIENT)
	EXPECT_EQ("601\n", Execute("find ", storage, " -type f | wc -l", 0));
  EXPECT_EQ("0\n", Execute("find ", lock_dir, " -type f | wc -l", 0));
#endif

#if 0
  //-lR | grep "^-" | wc -l
  std::ostringstream cmd;
#if 0
  cmd << "ls " << storage << '/'
      << prefix << " |  wc -l ";
#endif
  cmd << "ls " <<base::PathJoin(storage, prefix)
      << " | wc -l";
  int ret;
  char buf[4];
  const char *ebuf = "1";
  FILE *fp;
  ASSERT_TRUE((fp = popen(cmd.str().c_str(), "r")) != NULL);
  fgets(buf, sizeof(buf), fp);
  pclose(fp);
  buf[strlen(buf) - 1] = 0;
  ASSERT_EQ(1, atoi(buf));
#endif

  RestoreSetting();
}

//when the bundle number more than the MAX, user also can allocate one bundle
TEST(Bundle, AllocateMax) {
  const char *storage = "test";
  // write
  const char *content = "content of file";
  const int length = strlen(content);
	// EXPECT_EQ("601\n", Execute("find ", storage, " -type f | wc -l", 0));
	// Execute("rm -rf ", storage, 0);
}

//write base_len+ 'a', return url and len=write length
std::string RandomWrite(const char *prefix, const char *postfix
    , const char *storage, int base_len, int max_len, int *len) {
	const int length = base_len + rand() % max_len;
	bundle::Writer *writer = bundle::Writer::Allocate(prefix, postfix
		, length, storage);
	EXPECT_TRUE(NULL != writer);

	char *content = new char[length];
	memset(content, 'a', length);

	size_t written = 0;
	int ret = writer->Write(content,length, &written);
	EXPECT_EQ(0, ret);
	EXPECT_EQ(length, written);

	std::string url = writer->EnsureUrl();

	delete [] content;
	delete writer;

  *len = length;
	return url;
}

// when the number of allocated bundle larger than limit count perday,
// user can also allocate one
// Note: this test is not effective now, for the loop count in bundle.cc is
// always 1 or 2
TEST(Bundle, AllocateCapacity) {
	const char *storage = "test";
	const char *prefix = "donot-ls-here";
	const char *postfix = ".jpg";
  const char *lock_dir = ".lock";

	// empty storage first
	Execute("rm -rf ", storage, 0);
	Execute("rm -rf ", lock_dir, 0);

  const int base_len = 1024 * 10;
	const int max_len = 512;

#if 0
	// exceed the limit count 100
	bundle::Setting a_limited_setting = {
		bundle::kBundleHeaderSize + bundle::kFileHeaderSize + max_length
		, 100, 10,10, &bundle::ExtractSimple, &bundle::BuildSimple};
	bundle::SetSetting(a_limited_setting);
#endif

	const int COUNT = 600;
  int arr_length[COUNT];
  std::string url[COUNT];

	for (int i=0; i<COUNT; i++) {
    //printf("---number i %d\n", i);
		std::string u = RandomWrite(prefix, postfix, storage, base_len, max_len, &arr_length[i]);
		EXPECT_LT(10, u.size());
		url[i] = u;
	}

#if 0
	// set back to the default setting
	bundle::Setting default_setting = {
			2u * 1024 * 1024 * 1024,
      20000,
      50,
      4000, &bundle::ExtractSimple, &bundle::BuildSimple};
	bundle::SetSetting(default_setting);
#endif

#if 1
  //read
  int ret;
  for (int j=0; j<COUNT; j++) {
    std::string readed_content;
    ret = bundle::Reader::Read(url[j].c_str(), &readed_content, storage);
    ASSERT_EQ(0, ret) << "read" << "j:"<<j;
    ASSERT_EQ(arr_length[j], readed_content.size()) << "readed" << "j:"<<j;

    for (int c=0; c<arr_length[j]; ++c) {
      if (readed_content[c] != 'a') {
        ASSERT_TRUE(0);
        break;
      }
    }
  }
#endif


	//EXPECT_EQ("601\n", Execute("find ", storage, " -type f | wc -l", 0));
  //EXPECT_EQ("0\n", Execute("find ", lock_dir, " -type f | wc -l", 0));
	// Execute("rm -rf ", storage, 0);
}

// make sure one bundle is full before new bundle allocated
TEST(Bundle, AllocateSequence) {
  RestoreSetting();

	const char *storage = "test";
	const char *prefix = "donot-ls-here";
	const char *postfix = ".txt";
  const char *lock_dir = ".lock";

	// empty storage first
	Execute("rm -rf ", storage, 0);
	Execute("rm -rf ", lock_dir, 0);

	const int max_length = 1024 * 50;

	// max bundle filesize is 1K + 50K, write 2K (0.5K + 1500Byte) pertime,
  // one bundle(1K + 50K) can contain most 24 files. when exceed 25, will creat one new
  // bundle file
	bundle::Setting a_limited_setting = {
		bundle::kBundleHeaderSize + max_length, 100, 10,10
    , &bundle::ExtractSimple, &bundle::BuildSimple};
	bundle::SetSetting(a_limited_setting);

  std::string urls[30];

  int sum_length = 0;
	for (int i=0; i<30; i++) {
		const int length = 1500 + i;// + rand() % max_length;
    sum_length += bundle::Align1K(length + bundle::kFileHeaderSize);
		bundle::Writer *writer = bundle::Writer::Allocate(prefix, postfix
			, length, storage, lock_dir);
		ASSERT_TRUE(NULL != writer);

		char *content = new char[length];
		memset(content, 'a', length);

		size_t written = 0;
		int ret = writer->Write(content,length, &written);
		ASSERT_EQ(0, ret);
		EXPECT_EQ(length, written) ;

    urls[i] = writer->EnsureUrl();

    ASSERT_FALSE(urls[i].empty());

		delete [] content;
		delete writer;

#if !defined(USE_MOOSECLIENT)
	  if (i < 25)
      EXPECT_EQ("1\n", Execute("find ", storage, " -type f | wc -l", 0));
    else
      EXPECT_EQ("2\n", Execute("find ", storage, " -type f | wc -l", 0));
#endif
	}

  for (int j=0; j<30; j++) {
    std::string readed_content;
    int ret = bundle::Reader::Read(urls[j].c_str(), &readed_content, storage);
    ASSERT_EQ(0, ret) << "  j: " << j << " url:" << urls[j];
    ASSERT_EQ(1500+j, readed_content.size()) << " readed" << "j:"<<j;

    for (int c=0; c<1500+j; ++c) {
      if (readed_content[c] != 'a') {
        ASSERT_TRUE(0);
        break;
      }
    }
  }

  EXPECT_EQ("0\n", Execute("find ", lock_dir, " -type f | wc -l", 0));
  RestoreSetting();
}

// make sure when allocate the same file and write some buffer,
// the bundle can be allocated and no overwrite happend
TEST(Bundle, AllocateNonOverwrite) {
	const char *storage = "test";
	const char *prefix = "donot-ls-here";
	const char *postfix = ".txt";
  const char *lock_dir = ".lock";

	// empty storage first
	Execute("rm -rf ", storage, 0);
	Execute("rm -rf ", lock_dir, 0);

	const int max_length = 1024 * 100;

	// exceed the limit count 100
	bundle::Setting a_limited_setting = {
		bundle::kBundleHeaderSize + bundle::kFileHeaderSize + max_length
		, 100, 10,10, &bundle::ExtractSimple, &bundle::BuildSimple};
	bundle::SetSetting(a_limited_setting);

  int sum_length = 0;
	for (int i=0; i<29; i++) {
		const int length = 1024 + i*100;// + rand() % max_length;
    sum_length += bundle::Align1K(length + bundle::kFileHeaderSize);
		bundle::Writer *writer = bundle::Writer::Allocate(prefix, postfix
			, length, storage, lock_dir);
		ASSERT_TRUE(NULL != writer);

		char *content = new char[length];
		memset(content, 'a', length);

		size_t written = 0;
		int ret = writer->Write(content,length, &written);
		ASSERT_EQ(0, ret);
		EXPECT_EQ(length, written) ;

    // make sure the file size increase after write avoid overwrite
    // struct stat stat_buf;
    // stat((writer->bundle_file()).c_str(), &stat_buf);
    // int a = bundle::kBundleHeaderSize + sum_length;
    // ASSERT_EQ(a, stat_buf.st_size);
		delete [] content;
		delete writer;
	}

#if !defined(USE_MOOSECLIENT)
	EXPECT_EQ("1\n", Execute("find ", storage, " -type f | wc -l", 0));
  EXPECT_EQ("0\n", Execute("find ", lock_dir, " -type f | wc -l", 0));
#endif	
}

TEST(Bundle, WriteAndRead) {
  const char *storage = "test";
  int arr[] = {0, 1, 2, 1024, 1024+1, 4096, 10*1024
    , 10*1024*1024
    // TODO: more length
  };

  const char * prefix_arr[] = {"a", "a/b", "a/b/c", "/a/", "a/", "photo/20120512"}; 
    // TODO: 
    // , ""};

  for (int i=0; i<sizeof(arr)/sizeof(*arr); ++i) {
    for (int j=0; j<sizeof(prefix_arr)/sizeof(*prefix_arr); ++j) {
      bundle::Writer *writer = bundle::Writer::Allocate(prefix_arr[j], ".jpg"
          , arr[i], storage);

	    ASSERT_TRUE(0 != writer) << " > i:" << i <<", " <<"j:"<<j ;
      char *buf = new char[arr[i]];
      memset(buf, 'a', arr[i]);
      size_t written;
      int ret = writer->Write(buf, arr[i], &written);
      ASSERT_EQ(0, ret) << " > i:" << i <<", "<< "j:"<<j ;
      ASSERT_EQ(arr[i], written) << "written" <<" > i:" << i <<", "<< "j:"<<j;

      std::string url = writer->EnsureUrl();

      std::string readed_content;
      ret = bundle::Reader::Read(url, &readed_content, storage);
      ASSERT_EQ(0, ret) << "  " <<" i:" << i << " j:"<< j << " url:" << url;
      ASSERT_EQ(arr[i], readed_content.size()) << " i:" << i << " j:"<<j;

      for (int c=0; c<readed_content.size(); ++c) {
        if (readed_content[c] != 'a') {
          ASSERT_TRUE(0);
          break;
        }
      }

      delete [] buf;
      delete writer;
    }
  }
}


TEST(Bundle, filelock) {
	const char *lockfile[] = {
		".lock/a",
		".lock/aa",
		".lock/ab",
		".lock/aC",
		".lock/ad",
		".lock/b",
		".lock/bc",
		".lock/bd",
		".lock/c",
		".lock/ca",
		".lock/cb",
		".lock/cd",
		".lock/cc",
		".lock/d",
	};

  bundle::FileLock * filelock_a[14];
  bundle::FileLock * filelock_b[14];

	for (int i=0; i<sizeof(lockfile)/sizeof(*lockfile); ++i) {
    filelock_a[i] = new bundle::FileLock(lockfile[i]);
    EXPECT_TRUE(filelock_a[i]->TryLock());
	}
	for (int i=0; i<sizeof(lockfile)/sizeof(*lockfile); ++i) {
    filelock_b[i] = new bundle::FileLock(lockfile[i]);
    EXPECT_FALSE(filelock_b[i]->TryLock());
	}

#if !defined(USE_MOOSECLIENT)
  EXPECT_EQ("14\n", Execute("find ", ".lock", " -type f | wc -l", 0));
#endif

  for (int i=0; i<sizeof(lockfile)/sizeof(*lockfile); ++i) {
    delete filelock_a[i];
  }
#if !defined(USE_MOOSECLIENT)
  EXPECT_EQ("0\n", Execute("find ", ".lock", " -type f | wc -l", 0));
#endif

	for (int i=0; i<sizeof(lockfile)/sizeof(*lockfile); ++i) {
    EXPECT_TRUE(filelock_b[i]->TryLock());
	}
#if !defined(USE_MOOSECLIENT)
  EXPECT_EQ("14\n", Execute("find ", ".lock", " -type f | wc -l", 0));
#endif

	for (int i=0; i<sizeof(lockfile)/sizeof(*lockfile); ++i) {
    delete filelock_b[i];
  }
#if !defined(USE_MOOSECLIENT)
  EXPECT_EQ("0\n", Execute("find ", ".lock", " -type f | wc -l", 0));
#endif
}


struct foo2 {
  const char *prefix;
  int bid;
  size_t offset;
  size_t length;
  const char *postfix;
  const char *correct_url;
  const char content;
};

TEST(Bundle, AllocateLimit) {
  const char *storage = "test";
  const char *prefix = "donot-ls-here";
  const char *postfix = ".txt";
  const char *lock_dir = ".lock";

  const char *content = "content of file";
  const int length = strlen(content);

	// empty mock/ first
#if !defined(USE_MOOSECLIENT)
  Execute("rm -rf ", storage, 0);
  Execute("rm -rf ", lock_dir, 0);
#endif

	// exceed the limit count 100
	bundle::Setting a_limited_setting = {
		bundle::kBundleHeaderSize + bundle::kFileHeaderSize + length
			, 100, 10,10, &bundle::ExtractSimple, &bundle::BuildSimple};
	bundle::SetSetting(a_limited_setting);

  for (int i=0; i<600; i++) {
    //printf("---number : %d\n", i);
    bundle::Writer *writer = bundle::Writer::Allocate(prefix, postfix
        , length, storage, lock_dir);
    ASSERT_TRUE(NULL != writer);

		int ret = writer->Write(content,length);
		ASSERT_EQ(0, ret);

    delete writer;
  }
	// set back to the default setting
	bundle::Setting default_setting = {
			2u * 1024 * 1024 * 1024,
      20000,
      50,
      4000,
      &bundle::ExtractSimple, &bundle::BuildSimple};
	bundle::SetSetting(default_setting);

#if !defined(USE_MOOSECLIENT)
	EXPECT_EQ("600\n", Execute("find ", storage, " -type f | wc -l", 0));
  EXPECT_EQ("0\n", Execute("find ", lock_dir, " -type f | wc -l", 0));
#endif
}

TEST(Bundle, AllocateDefault) {
  const char *storage = "test";
  const char *prefix = "donot-ls-here";
  const char *postfix = ".txt";
  const char *lock_dir = ".lock";

  const char *content = "content of file";
  const int length = strlen(content);

	// empty mock/ first
#if !defined(USE_MOOSECLIENT)
  Execute("rm -rf ", storage, 0);
  Execute("rm -rf ", lock_dir, 0);
#endif

  for (int i=0; i<600; i++) {
    //printf("---number : %d\n", i);
    bundle::Writer *writer = bundle::Writer::Allocate(prefix, postfix
        , length, storage, lock_dir);
    ASSERT_TRUE(NULL != writer);

		int ret = writer->Write(content,length);
		ASSERT_EQ(0, ret);

    delete writer;
  }

#if !defined(USE_MOOSECLIENT)
	EXPECT_EQ("1\n", Execute("find ", storage, " -type f | wc -l", 0));
  EXPECT_EQ("0\n", Execute("find ", lock_dir, " -type f | wc -l", 0));
#endif
}

TEST(Bundle, WriteRandom) {
  const char *storage = "test";
  const char *prefix = "donot-ls-here";
  const char *postfix = ".txt";
  const char *lock_dir = ".lock";

  int length = 1024 * 10;
  int times = 100;
	char *content = new char[length];
	memset(content, 'a', length);

	// empty mock/ first
#if !defined(USE_MOOSECLIENT)
  Execute("rm -rf ", storage, 0);
  Execute("rm -rf ", lock_dir, 0);
#endif

  for (int i=0; i<times; i++) {
    //printf("---number : %d\n", i);
    bundle::Writer *writer = bundle::Writer::Allocate(prefix, postfix
        , length, storage, lock_dir);
    ASSERT_TRUE(NULL != writer);

		int ret = writer->Write(content,length);
		ASSERT_EQ(0, ret);

    delete writer;
  }

#if !defined(USE_MOOSECLIENT)
	EXPECT_EQ("1\n", Execute("find ", storage, " -type f | wc -l", 0));
  EXPECT_EQ("0\n", Execute("find ", lock_dir, " -type f | wc -l", 0));
#endif
}


bundle::Info info_;

std::string FakeBuild(const bundle::Info &info) {
  info_ = info;
  return "fake";
}

bool FakeExtract(const char *url, bundle::Info *info) {
  *info = info_;
  return true;
}

// user define Url build and extract function
TEST(Bundle, UserDefinedUrl) {
  const char *storage = "test";

  const char *content = "content of file";
  const int length = strlen(content);

  std::string url;

  // write
  {
    bundle::Writer *writer = bundle::Writer::Allocate("p/20120512", ".txt"
      , length, storage, 0, &FakeBuild);
	  ASSERT_TRUE(0 != writer);

    url = writer->EnsureUrl();
    EXPECT_EQ(url, "fake") << url;
    
    int ret = writer->Write(content, length);
    ASSERT_EQ(0, ret);
    delete writer;
  }

  // read
  bundle::Info info;
  bool f = FakeExtract(url.c_str(), &info);
  EXPECT_TRUE(f) << " prefix:" << info.prefix << " id:" << info.id 
    << " offset:" << info.offset << " size:" << info.size;

  std::string readed_content;
  ASSERT_EQ(0, bundle::Reader::Read(url, &readed_content, storage, &FakeExtract));
  ASSERT_EQ(readed_content, content);
}

struct Foo {
  std::string url;
  std::string content;
};

TEST(Bundle, BatchWrite) {
  const char *storage = "test";

  std::vector<Foo> foos;
  int arr[] = {1, 10, 100, 1023, 1024, 1025, 1024*4, 1024*4+1, 1024*1024
    , 1024*1024 + 1, 1024*1024 + 100, 1024*1024*64-1, 1024*1024*64, 1024*1024*64+1
  };

  {
    bundle::Writer *writer = bundle::Writer::Allocate("p/20120512", ".txt"
      , 10*1024, storage, 0);
    ASSERT_TRUE(0 != writer);

    for (int i=0; i<sizeof(arr)/sizeof(*arr); ++i) {
      Foo foo;
      std::string content = std::string(arr[i], 'a' + i);
      size_t written = 0;
      std::string url;

      int ret = writer->BatchWrite(content.data(), content.size(), &written, &foo.url);
      ASSERT_EQ(0, ret) << "write size: " << arr[i];
      ASSERT_EQ(content.size(), written);
      ASSERT_TRUE(!foo.url.empty());
      
      foos.push_back(foo);
    }

    delete writer;
  }

  for (int i=0; i<sizeof(arr)/sizeof(*arr); ++i) {
    const Foo & foo = foos[i];
    std::string content = std::string(arr[i], 'a' + i);
    std::string readed_content;
    ASSERT_EQ(0, bundle::Reader::Read(foo.url, &readed_content, storage)) << foo.url;
    ASSERT_EQ(readed_content, content);
  }
}
