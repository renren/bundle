Why Bundle?
---------------------
Bundle is a storage solution for huge amount of small files.

Nearly all native filesytems are very limited in the too-many-small-files scenario, and distributed filesystems
(DFS) are optimized for large files.

The most serious problem with titanic amount of small files storage is data transition. Data transition occurs
when
- Duplicate or backup data
- Restore data from backup
- Restore duplication level when replica or backup failed

When store small files as is in traditional filesystem or distributed filesystem, each file has its seperate meta
data. So, during data transition, for every file to be duplicated, backuped or restored, operation to meta data and
data itself are needed, and this contributes to titanic amount of random access, thus performance is seriously hurt,
and transition is very very very slow.

For offline, batching usage model, the bad performance may be acceptable. But for online usage model, data
transition in question often leads to service problems. Some IO bandwidth throttle mechanisms can be applied to
mitigate service problems, but increasing transition time which is already too long means higher data loss risk.

There are a few DFS' address this issue purposely. For example,
- Haystack of Facebook Inc. (http://www.facebook.com/note.php?note_id=76191543919)
- TFS of Tabao Inc. (http://code.taobao.org/p/tfs/src)

They both pack small files into large chunks, with one key difference: adressing information,
- Haystack encodes addressing information into file URI, avoid index, but doesn't support overwrite. Files can
only be written once.
- TFS store addressing information in index, and load it in memory. It supports overwrite.

Then why Bundle?

Haystack is not open sourced. TFS is open source, but it lacks of customized URI prefix. For URI pattern based 
CDN tuning, customized URI prefix is necessary.

Bundle addresses this very issue by enabling customizable prefix in file URI, currently date time, but other
pattern will be added if needed.

Bundle layers upon other DFS. Actually, it functions as a URI name service. By using it, applications can store
small files in large files on DFS, and the underlying DFS handles data transition at the best performance, 
a.k.a Large Is Better. Then, the only concern is choosing a underlying DFS suitable for you, for instance,
supporting remote location replica or not.

Currently, [Moosefs](http://www.moosefs.org) is our recommendation.


How it works?
---------------------
Bundle helps packing a lot of small files to a single large file. Later, access them by bundle filename, offset and
size.

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

The large file (bundle) p/20120331/00000001/00000039/00000039 size is 2.0G, contain thousands of small file.

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
  const char *file_buffer = "content of file";
  const int file_size = strlen(file_buffer);

  Writer *writer = Writer::Allocate("p/20120512", ".jpg", file_size, "/mnt/mfs");

  std::cout << "url: " << writer->EnsureUrl() << std::endl;

  size_t written;
  int ret = writer->Write(file_buffer, file_size, &written);

  writer->Release(); // or delete writer
```

The url like:
  p/20120512/4,2048,15.jpg


Read:
```
  std::string buf;
  int ret = bundle::Reader::Read("p/20120512/4,2048,15.jpg", &buf, "/mnt/mfs");
```

Build:
```
sudo apt-get install gyp
git clone git://github.com/xiaonei/bundle.git bundle.git
cd bundle.git
gyp --depth=. --toplevel-dir=. -Dlibrary=static_library gyp/bundle.gyp
make
ls out/Default
```

Deploy web access
----------------------------
Use nginx and fastcgi provide web access.

-  Start the daemon, this is fastcgi back-end of nginx  
out/Default/bundled -p 9000 -F 40 -d /mnt/mfs

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

