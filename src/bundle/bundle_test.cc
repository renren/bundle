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
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include "gtest/gtest.h"

#include "bundle/bundle.h"
#include "base3/pathops.h"
#include "bundle/filelock.h"

struct foo {
  const char *prefix;
  int bid;
  size_t offset;
  size_t length;
  const char *postfix;
  const char *correct_url;
};


TEST(Bundle, URL) {
  foo arr[] = {
    {"a", 1, 100, 100, ".jpg", "a/B/Bq/Bq/EV7kdt.jpg"},
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
  };

  for (int i=0; i<sizeof(arr)/sizeof(*arr); ++i) {
    foo *f = &arr[i];
    std::string url = bundle::BuildSimple(f->bid, f->offset, f->length,
      f->prefix, f->postfix);

#if 0
		std::cout << url << "\n";
#endif

    std::string expected_filename = bundle::Bid2Filename(f->bid);

    std::string extracted_file;
    size_t extracted_offset, extracted_size;

    bool flag = bundle::ExtractSimple(url.c_str(), &extracted_file
        , &extracted_offset, &extracted_size);
		EXPECT_TRUE(flag) << "  > url: " << url << " #" << i;

    EXPECT_EQ(base::PathJoin(f->prefix,expected_filename), extracted_file);
    EXPECT_EQ(f->offset, extracted_offset);
    EXPECT_EQ(f->length, extracted_size);

		if (f->correct_url)
			EXPECT_EQ(f->correct_url, url) << " > correct is:" << f->correct_url;

		// part check
		flag = bundle::ExtractSimple(url.c_str(), NULL, NULL, NULL);
		EXPECT_TRUE(flag);

		// hash change
		std::string ua = url;
		ua.insert(ua.size() - 4, "e");
		EXPECT_FALSE(bundle::ExtractSimple(ua.c_str(), NULL, NULL, NULL));

		ua = url;
		ua.append("e");
		EXPECT_FALSE(bundle::ExtractSimple(ua.c_str(), NULL, NULL, NULL));

		std::string::size_type pos = ua.size();
		int c = 3;
		while (c--) {
			ua = url;
			pos = ua.rfind("/", pos - 1);
			ua[pos - 1] ++;
			EXPECT_FALSE(bundle::ExtractSimple(ua.c_str(), NULL, NULL, NULL));
		}
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
		EXPECT_FALSE(bundle::ExtractSimple(us[i], NULL, NULL, NULL));
	}
}

TEST(Bundle, Example) {
  const char *storage = "bigpool/p";
  const char *lockdir = "examplelock";

  // write
  const char *content = "content of file";
  const int length = strlen(content);

  bundle::Writer *writer = bundle::Writer::Allocate("p/20120512", ".txt"
      , length, storage);
	ASSERT_TRUE(0 != writer);

  size_t written;
  int ret = writer->Write(content, length, &written);
  ASSERT_EQ(0, ret);

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
  const char *storage = "mock";
  const char *prefix = "donot-ls-here";
  const char *postfix = ".txt";
  const char *lock_dir = "lock_reuse";

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

		EXPECT_EQ("1\n", Execute("find ", storage, " -type f | wc -l", 0));

		delete writer;

    EXPECT_EQ("0\n", Execute("find ", lock_dir, " -type f | wc -l", 0));
	}

	// allocate again, without write, make sure no more bundle created
	{
		bundle::Writer *writer = bundle::Writer::Allocate(prefix, postfix
			, length, storage, lock_dir);
		EXPECT_TRUE(writer != NULL);

		EXPECT_EQ("1\n", Execute("find ", storage, " -type f | wc -l", 0));

		delete writer;
    EXPECT_EQ("0\n", Execute("find ", lock_dir, " -type f | wc -l", 0));
	}

	// exceed the limit count 100
	bundle::Setting a_limited_setting = {
		bundle::kBundleHeaderSize + bundle::kFileHeaderSize + length
			, 100, 10,10};
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

	EXPECT_EQ("601\n", Execute("find ", storage, " -type f | wc -l", 0));
  EXPECT_EQ("0\n", Execute("find ", lock_dir, " -type f | wc -l", 0));
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
}

//when the bundle number more than the MAX, user also can allocate one bundle
TEST(Bundle, AllocateMax) {
  const char *storage = "bigpool/p";
  // write
  const char *content = "content of file";
  const int length = strlen(content);
	//EXPECT_EQ("601\n", Execute("find ", storage, " -type f | wc -l", 0));
	//Execute("rm -rf ", storage, 0);
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
	const char *storage = "capacity";
	const char *prefix = "donot-ls-here";
	const char *postfix = ".jpg";
  const char *lock_dir = "lock_capacity";

	// empty storage first
	Execute("rm -rf ", storage, 0);
	Execute("rm -rf ", lock_dir, 0);

  const int base_len = 1024 * 10;
	const int max_len = 512;

#if 0
	// exceed the limit count 100
	bundle::Setting a_limited_setting = {
		bundle::kBundleHeaderSize + bundle::kFileHeaderSize + max_length
		, 100, 10,10};
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
      4000};
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
	const char *storage = "Squence";
	const char *prefix = "donot-ls-here";
	const char *postfix = ".txt";
  const char *lock_dir = "lock_sequence";

	// empty storage first
	Execute("rm -rf ", storage, 0);
	Execute("rm -rf ", lock_dir, 0);

	const int max_length = 1024 * 50;

	// max bundle filesize is 1K + 50K, write 2K (0.5K + 1500Byte) pertime,
  // one bundle(1K + 50K) can contain most 24 files. when exceed 25, will creat one new
  // bundle file
	bundle::Setting a_limited_setting = {
		bundle::kBundleHeaderSize + max_length
		, 100, 10,10};
	bundle::SetSetting(a_limited_setting);

  std::string url[30];

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

    url[i] = writer->EnsureUrl();

		delete [] content;
		delete writer;
	  if (i < 25)
      EXPECT_EQ("1\n", Execute("find ", storage, " -type f | wc -l", 0));
    else
      EXPECT_EQ("2\n", Execute("find ", storage, " -type f | wc -l", 0));
	}

  //read
  int ret;
  for (int j=0; j<30; j++) {
    std::string readed_content;
    ret = bundle::Reader::Read(url[j].c_str(), &readed_content, storage);
    ASSERT_EQ(0, ret) << "read" << "j:"<<j;
    ASSERT_EQ(1500+j, readed_content.size()) << "readed" << "j:"<<j;

    for (int c=0; c<1500+j; ++c) {
      if (readed_content[c] != 'a') {
        ASSERT_TRUE(0);
        break;
      }
    }
  }

  EXPECT_EQ("0\n", Execute("find ", lock_dir, " -type f | wc -l", 0));
}

// make sure when allocate the same file and write some buffer,
// the bundle can be allocated and no overwrite happend
TEST(Bundle, AllocateNonOverwrite) {
	const char *storage = "overwrite";
	const char *prefix = "donot-ls-here";
	const char *postfix = ".txt";
  const char *lock_dir = "lock_overwrite";

	// empty storage first
	Execute("rm -rf ", storage, 0);
	Execute("rm -rf ", lock_dir, 0);

	const int max_length = 1024 * 100;

	// exceed the limit count 100
	bundle::Setting a_limited_setting = {
		bundle::kBundleHeaderSize + bundle::kFileHeaderSize + max_length
		, 100, 10,10};
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
    struct stat stat_buf;
    stat((writer->bundle_file()).c_str(), &stat_buf);
    int a = bundle::kBundleHeaderSize + sum_length;
    ASSERT_EQ(a, stat_buf.st_size);
		delete [] content;
		delete writer;
	}

	EXPECT_EQ("1\n", Execute("find ", storage, " -type f | wc -l", 0));
  EXPECT_EQ("0\n", Execute("find ", lock_dir, " -type f | wc -l", 0));
	// Execute("rm -rf ", storage, 0);
}

TEST(Bundle, Threads) {
	// create threads ...
	// random
	// save url
}

TEST(Bundle, WriteAndRead) {
  const char *storage = "";
  int arr[] = {0, 1, 2, 1024, 1024+1, 4096, 10*1024
    , 10*1024*1024
    // TODO: more length
  };

  const char * prefix_arr[] = {"", "a", "a/b", "a/b/c", "/a/", "a/", "photo/20120512"};

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
      ASSERT_EQ(0, ret) << "read" <<" > i:" << i <<", "<< "j:"<<j;
      ASSERT_EQ(arr[i], readed_content.size()) << "readed" <<" > i:" << i <<", "<< "j:"<<j;

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

  EXPECT_EQ("14\n", Execute("find ", ".lock", " -type f | wc -l", 0));

  for (int i=0; i<sizeof(lockfile)/sizeof(*lockfile); ++i) {
    delete filelock_a[i];
  }
  EXPECT_EQ("0\n", Execute("find ", ".lock", " -type f | wc -l", 0));

	for (int i=0; i<sizeof(lockfile)/sizeof(*lockfile); ++i) {
    EXPECT_TRUE(filelock_b[i]->TryLock());
	}

  EXPECT_EQ("14\n", Execute("find ", ".lock", " -type f | wc -l", 0));

	for (int i=0; i<sizeof(lockfile)/sizeof(*lockfile); ++i) {
    delete filelock_b[i];
  }

  EXPECT_EQ("0\n", Execute("find ", ".lock", " -type f | wc -l", 0));
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

foo2 arr_foo2[] = {
  {"a", 1, 100, 100, ".jpg", "a/B/Bq/Bq/EV7kdt.jpg", 'a'},
  {"a", 2, 100, 100, ".jpg", "a/C/Bq/Bq/CR33M9.jpg", 'b'},
  {"a/b", 1024, 100, 100, ".jpg", "a/b/SE/Bq/Bq/Xtc8w.jpg", 'c'},
  {"a/b/c", 20000, 100, 100, ".jpg", "a/b/c/FjV/Bq/Bq/CpF9ow.jpg", 'd'},
  {"a/b/c", 3, 0, 0, ".jpg", "a/b/c/D/A/A/BbKKbQ.jpg", 'e'},
  {"a/b/c", 16, 0, 1, ".jpg", "a/b/c/R/A/B/EmeovP.jpg", 'f'},
  {"a/b/c", 3, 0, 12983712, ".jpg", "a/b/c/D/A/BAGlN/EXE2Zz.jpg", 'g'},
  {"a/b/c", 3, 2348924, 12983712, ".jpg", "a/b/c/D/L2du/BAGlN/EcgZrK.jpg", 'h'},
  {"n", 0, 0, 0, ".jpg", "n/A/A/A/oaK13.jpg", 'i'},
  {"n", 0, 100, 100, ".jpg", "n/A/Bq/Bq/D3oYBN.jpg", 'j'},
};

const char *pro_storage = "write_proc";
const char *pro_lock_dir = "lock_proc";


void Write_Proc(std::string *url) {
  //std::string url[10];
  for (int i=0; i<sizeof(arr_foo2)/sizeof(*arr_foo2); ++i) {

    foo2 *f = &arr_foo2[i];
    bundle::Writer *writer = bundle::Writer::Allocate(f->prefix, f->postfix
          , f->length, pro_storage, pro_lock_dir);

    ASSERT_TRUE(0 != writer) << "i = " <<i;
    char *buf = new char[f->length];
    memset(buf, f->content, f->length);
    size_t written;
    int ret = writer->Write(buf, f->length, &written);
    ASSERT_EQ(0, ret);
    ASSERT_EQ(f->length, written);

    url[i] = writer->EnsureUrl();
    //printf("---%d,url is %s\n",getpid(), url[i].c_str());
    delete [] buf;
    delete writer;
  }
    //return &url;
}

void Write_Procfile(char *filename) {
  int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC,
      S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  for (int i=0; i<sizeof(arr_foo2)/sizeof(*arr_foo2); ++i) {
    foo2 *f = &arr_foo2[i];
    bundle::Writer *writer = bundle::Writer::Allocate(f->prefix, f->postfix
          , f->length, pro_storage, pro_lock_dir);

    ASSERT_TRUE(0 != writer) << "i = " <<i;
    char *buf = new char[f->length];
    memset(buf, f->content, f->length);
    size_t written;
    int ret = writer->Write(buf, f->length, &written);
    ASSERT_EQ(0, ret);
    ASSERT_EQ(f->length, written);

    std::string url = writer->EnsureUrl();
    if (-1 == (ret = write(fd, url.c_str(), url.length())))
      printf("Error, write file\n");
    write(fd, "\n", 1);
    //printf("---%d,url is %s\n",getpid(), url.c_str());
    delete [] buf;
    delete writer;
  }
  close(fd);
}

void Read_Proc(std::string *url) {
  for (int i=0; i<sizeof(arr_foo2)/sizeof(*arr_foo2); ++i) {
    //printf("---%d read url is %s\n", getpid(), url[i].c_str());
    int ret;
    foo2 *f = &arr_foo2[i];
    std::string readed_content;
    ret = bundle::Reader::Read(url[i], &readed_content, pro_storage);
    ASSERT_EQ(0, ret) << "read i : " <<i << " url :" << url[i];
    ASSERT_EQ(f->length, readed_content.size());

    for (int c=0; c<readed_content.size(); ++c) {
      if (readed_content[c] != f->content) {
        ASSERT_TRUE(0);
        break;
      }
    }
  }
}

std::string arr_url[10];
std::string arr_url2[10];

//multi-process test used fork()
//two processes(parent and child) write arr_foo2->length*arr_foo2->content;
//two processes(parent and child) read and check
TEST(Bundle, Process) {
	Execute("rm -rf ", pro_storage, 0);
	Execute("rm -rf ", pro_lock_dir, 0);

  //write process
  int f;
  ASSERT_FALSE((f = fork()) < 0);
  if (f>0) {
    Write_Procfile("filename.txt");
    int stat_child;
    waitpid(f, &stat_child, 0);
    if (WIFEXITED(stat_child))
      printf("write Chile exit code is %d\n", WEXITSTATUS(stat_child));
    else if (WIFSIGNALED(stat_child))
      printf("write Child terminated abnormally, signal %d\n", WTERMSIG(stat_child));
  }
  else {
    Write_Procfile("filename2.txt");
    exit(EXIT_SUCCESS);
  }

  //get url to arr_url[] and arr_url2[]
  char name[100];
  FILE * fp;
  fp=fopen("filename.txt", "r+");
  for (int i=0; i<sizeof(arr_foo2)/sizeof(*arr_foo2); ++i) {
        fgets(name, 100, fp);
        name[strlen(name)-1] =0;
        arr_url[i].append(name);
      }
  fclose(fp);
  fp=fopen("filename2.txt", "r+");
  for (int i=0; i<sizeof(arr_foo2)/sizeof(*arr_foo2); ++i) {
        fgets(name, 100, fp);
        name[strlen(name)-1] =0;
        arr_url2[i].append(name);
      }
  fclose(fp);

  //arr_url2[4] = "a/b/c/FjV/Bq/Bq/CpF9ow.jpg";
  //read process
  ASSERT_FALSE((f = fork()) < 0);
  if (f>0) {
    Read_Proc(arr_url);
    int stat_child;
    waitpid(f, &stat_child, 0);
    if (WIFEXITED(stat_child))
      printf("read Chile exit code is %d\n", WEXITSTATUS(stat_child));
    else if (WIFSIGNALED(stat_child))
      printf("read Child terminated abnormally, signal %d\n", WTERMSIG(stat_child));
  }
  else {
    Read_Proc(arr_url2);
    exit(EXIT_SUCCESS);
  }
}

TEST(Bundle, AllocateLimit) {
  const char *storage = "mock";
  const char *prefix = "donot-ls-here";
  const char *postfix = ".txt";
  const char *lock_dir = "lock_Reuse";

  const char *content = "content of file";
  const int length = strlen(content);

	// empty mock/ first
  Execute("rm -rf ", storage, 0);
  Execute("rm -rf ", lock_dir, 0);

	// exceed the limit count 100
	bundle::Setting a_limited_setting = {
		bundle::kBundleHeaderSize + bundle::kFileHeaderSize + length
			, 100, 10,10};
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
      4000};
	bundle::SetSetting(default_setting);

	EXPECT_EQ("600\n", Execute("find ", storage, " -type f | wc -l", 0));
  EXPECT_EQ("0\n", Execute("find ", lock_dir, " -type f | wc -l", 0));
}

TEST(Bundle, AllocateDefault) {
  const char *storage = "dmock";
  const char *prefix = "donot-ls-here";
  const char *postfix = ".txt";
  const char *lock_dir = "lock_reuse";

  const char *content = "content of file";
  const int length = strlen(content);

	// empty mock/ first
  Execute("rm -rf ", storage, 0);
  Execute("rm -rf ", lock_dir, 0);

  for (int i=0; i<600; i++) {
    //printf("---number : %d\n", i);
    bundle::Writer *writer = bundle::Writer::Allocate(prefix, postfix
        , length, storage, lock_dir);
    ASSERT_TRUE(NULL != writer);

		int ret = writer->Write(content,length);
		ASSERT_EQ(0, ret);

    delete writer;
  }

	EXPECT_EQ("1\n", Execute("find ", storage, " -type f | wc -l", 0));
  EXPECT_EQ("0\n", Execute("find ", lock_dir, " -type f | wc -l", 0));
}

TEST(Bundle, WriteRandom) {
  const char *storage = "random";
  const char *prefix = "donot-ls-here";
  const char *postfix = ".txt";
  const char *lock_dir = "lock_reuse";

  int length = 1024 * 10;
  int times = 100;
	char *content = new char[length];
	memset(content, 'a', length);

	// empty mock/ first
  Execute("rm -rf ", storage, 0);
  Execute("rm -rf ", lock_dir, 0);

  for (int i=0; i<times; i++) {
    //printf("---number : %d\n", i);
    bundle::Writer *writer = bundle::Writer::Allocate(prefix, postfix
        , length, storage, lock_dir);
    ASSERT_TRUE(NULL != writer);

		int ret = writer->Write(content,length);
		ASSERT_EQ(0, ret);

    delete writer;
  }

	EXPECT_EQ("1\n", Execute("find ", storage, " -type f | wc -l", 0));
  EXPECT_EQ("0\n", Execute("find ", lock_dir, " -type f | wc -l", 0));
}



std::string FakeBuild(uint32_t bid, size_t offset, size_t length
    , const char *prefix, const char *postfix) {
  std::ostringstream ostem;
  ostem << prefix << "/" << bundle::Bid2Filename(bid) << ","
    << offset << ","
    << length;
  return ostem.str();
}

bool FakeExtract(const char *url, std::string *bundle_name, size_t *offset
    , size_t *length) {
  std::vector<std::string> vs;
  boost::split(vs, url, boost::is_any_of(","));

  // uint32_t id = boost::lexical_cast<size_t>(vs[0]);
  *bundle_name = vs[0];
  if (offset)
    *offset = boost::lexical_cast<size_t>(vs[1]);
  if (length)
    *length = boost::lexical_cast<size_t>(vs[2]);
  return true;
}

// user define Url build and extract function
TEST(Bundle, UserDefinedUrl) {
  const char *storage = "bigpool/p";

  const char *content = "content of file";
  const int length = strlen(content);

  std::string url;

  // write
  {
    bundle::Writer *writer = bundle::Writer::Allocate("p/20120512", ".txt"
      , length, storage, 0, &FakeBuild);
	  ASSERT_TRUE(0 != writer);

    url = writer->EnsureUrl();
    
    int ret = writer->Write(content, length);
    ASSERT_EQ(0, ret);
    delete writer;
  }

  // read
  std::string bn;
  bool f = FakeExtract(url.c_str(), &bn, 0, 0);
  EXPECT_TRUE(f) << bn;

  std::string readed_content;
  ASSERT_EQ(0, bundle::Reader::Read(url, &readed_content, storage, &FakeExtract));
  ASSERT_EQ(readed_content, content);
}
