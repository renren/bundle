What is Bundle?
---------------------
Bundle is a storage solution for huge mount of small files.

As known native filesytem is very limited in the secenario. Common DFS also failed.

[Moosefs](http://www.moosefs.org) is our recommendation.

How it works?
---------------------
Bundle small files to a single file. Access them via bundle file name, offset and size.

For example:
<pre>
# tree .
.
└── p
    ├── 20120331
    │   ├── 00000001
    │   │   ├── 00000039
    │   │   ├── 0000003a
    │   │   ├── 0000003b

# ls -lh p/20120331/00000001/00000039
-rw-rw-r-- 1 root root  2.0G Mar 31 20:36 00000039
-rw-rw-r-- 1 root root  2.0G Mar 31 20:49 0000003a
-rw-rw-r-- 1 root root  2.0G Mar 31 20:47 0000003b
-rw-rw-r-- 1 root root  2.0G Mar 31 20:12 0000003c
-rw-rw-r-- 1 root root  2.0G Mar 31 20:38 0000003d
</pre>

The huge file(bundle) p/20120331/00000001/00000039/00000039 size is 2.0G, contain thounds of small file.

The bundle format:
<pre>
 ---------------------
|                     |
| bundle header       |
|---------------------|
| file header         |
|   url, date, flag...|
|---------------------|
| file content        |
| ... ...             |
|                     |
|---------------------|
| file header         |
|   url, date, flag...|
|---------------------|
| file content        |
| ... ...             |
|                     |
|---------------------|
...
</pre>


How to use?
---------------------
Include the file
```
#include "bundle/bundle.h"
```

Write:
```
  char *file_buffer = "content of file";
  const int length = strlen(file_buffer);

  Writer *writer = Writer::Allocate("p/20120512", length, "/mnt/mfs");

  size_t written;
  int ret = writer->Write(file_buffer, length, &written);
  if (0 == ret)
    std::cout << "write success, url:" << writer->EnsureUrl() << std::endl;

  writer->Release(); // or delete writer
```

The url like:
  p/20120424/E6/SE/Zb1/C5zWed.jpg


Read:
```
  std::string buf;
  int ret = bundle::Reader::Read("p/20120424/E6/SE/Zb1/C5zWed.jpg", &buf, "/mnt/mfs");
```

Build:
```
git clone git://github.com/xiaonei/bundle.git
cd bundle
gyp --depth=. --toplevel-dir=. -Dlibrary=static_library gyp/bundle.gyp
make
```

More
----------------------------
Use nginx and fastcgi provide web access.

-  Start the fastcgi as daemon, this is backend of nginx  
src/bundle/bundled -p 9000 -F 40 -d /mnt/mfs

-  Change Ningx configure file, add:  
```
# upload
location ~ bundle {
    client_max_body_size 20M;
    fastcgi_pass 127.0.0.1:9000;
    include fastcgi_params;
}
```
```
# read
location /p/ {
    fastcgi_pass 127.0.0.1:9000;
    include fastcgi_params;
}
```

-  Save content below as file, and open in browser:  
```
<form action="http://127.0.0.1/bundle"
    enctype="multipart/form-data"
    method="post">
  <input type="file" name="name_of_files" /><br/>
  <input type=submit />
</form>
```
