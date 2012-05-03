# coding=utf-8

import os
import errno
import time
import struct
import logging
import random

import murmur
import sixty
from filelock import FileLock

kFILE_NORMAL = 1
kFILE_MARK_DELETE = 2
kFILE_REDIRECT = 4
kMAGIC = 0xE4E4E4E4

"""
  header (16 byte)
---------------------------------------
E4E4E4E4    4byte: magic, folk by qinghui
length      4byte: include header
version     4byte: 1
flag        4byte: 1,2,4


"""

def read(place, url, check_flag=False):
    o = extract_url(url)
    if o is None: 
        raise Exception('url illegal')

    fn = bundle_filename(o['bid'], place)
    f = open(fn, 'rb') # TODO: error fix
    f.seek(o['offset'], os.SEEK_SET)
    # 1 read header
    header = f.read(kFileHeaderSize)
    fmt = 'IIII'
    magic, length, version, flag = struct.unpack(fmt, header[:4*4])
        
    if magic != kMAGIC:
        raise Exception('magic failed')
    if length - kFileHeaderSize != o['length']:
        raise Exception('length match failed')
    if version != 1:
        raise Exception('version not supported')
    if check_flag and flag != kFILE_NORMAL:
        raise Exception('flag not normal %d' % flag)

    return f.read(length - kFileHeaderSize)

class Writer(object):
    def __init__(self, file_lock, filename, bid, offset, length):
        self.flock = file_lock
        self.filename = filename
        self.bid = bid
        self.offset = offset
        self.length = length

        assert self.offset >= kBundleHeaderSize

    def __del__(self):
        self.flock.release()

    def ensure_url(self, prefix, postfix):
        # (prefix, bid, offset, length, postfix)
        self.url = build_url(prefix, bid=self.bid, offset=self.offset
                             , length=self.length, postfix=postfix)

    def write(self, content, userdata=None):
        # 0 
        if getattr(self, 'url', None) is None:
            raise Exception('call ensure_url first')
            return 0

        # 1 open
        fp = open(self.filename, 'r+b')
        fp.seek(self.offset, os.SEEK_SET)
        assert fp.tell() == self.offset
        
        if False:
            o = extract_url(self.url)
            assert isinstance(o, dict)
            # assert o['bid'] == self.bid, '%r %s' % (o, self.bid)
            assert o["offset"] == self.offset
            assert o["length"] == self.length
            assert self.length == len(content)

        # 2 write header
        # 保证上次写入在文件末尾追加了足够的 chr(0)
        assert self.offset == align1k(self.offset), self.offset

        to_write_array = []
        to_write_array.append(struct.pack("IIII", kMAGIC, 
                                  self.length + kFileHeaderSize, 
                                  1,    # version
                                  kFILE_NORMAL     # flag
                                  ))
        to_write_array.append(self.url.ljust(256, chr(0)))

        # TODO: userdata
        to_write_array.append(''.ljust(256 - 4 * 4, chr(0)))
        # assert sum(map(len, to_write_array)) == kFileHeaderSize

        # 补齐到 1k
        padding = align1k(kFileHeaderSize + len(content)) - kFileHeaderSize
        # print 'padding size:', padding - len(content)
        to_write_array.append(content.ljust(padding, chr(0)))

        huge = ''.join(to_write_array)
        # assert sum(map(len, to_write_array)) == len(huge)

        fp.write(huge)
        pos = fp.tell()
        fp.close()

        if pos == self.offset + len(huge):
            return self.length
        else:
            return 0

kBundleCountPerDay = 50 * 400           # 每天分配 2T 的空间
kMaxBundleSize = 2 * 1024 * 1024 * 1024 # 2G
kBundleHeaderSize = 1024
kFileHeaderSize = 512
_last_id = None # 上次使用过的

_lock_place = '/mnt/mfs/.lock'

def allocate(length, place, tm=None):
    """
    1 最小代价找到一个可用文件, 从 _last_id 开始找起 -1 == stat('%d.lock')
    2 try lock
    3 stat again

    # 应对一个目录下文件数目太多问题
    - 在url中携带 bundle path 信息，并加密

    data = open('sample.jpg', 'rb').read()

    w = bundle.allocate(len(data), 'p')
    w.ensure_url(prefix='fmn04', postfix='.jpg')
    w.write(data)

    del w   # important to release the file lock

    >>> if not os.path.exists('p'): os.makedirs('p')
    >>> if os.path.exists('p/20110920/A'): os.unlink('p/20110920/A')
    >>> data = open('sample.jpg', 'rb').read()
    >>> w = allocate(len(data), place='p', tm=time.strptime('20110919', '%Y%m%d'))
    >>> w.ensure_url(prefix='fmn04', postfix='.jpg')
    >>> w.write(data)
    83283
    """

    global _last_id
    if _last_id is None:
        _last_id = os.getpid() % 10

    if tm is None:
        tm = time.localtime()

    loop_count = 0

    while True:
        # 0
        # 检测是否循环了一圈，采用备用方案: 直接分配一个超过 kBundleCountPerDay 的文件
        loop_count += 1
        if loop_count >= kBundleCountPerDay:
            logging.info('allocation policy loop failed')
            _last_id = kBundleCountPerDay + random.randint(1, 100)

        # 1
        bid = new_bid(_last_id % kBundleCountPerDay, tm)
        fn_bundle = bundle_filename(bid, place)
        try:
            statinfo = os.stat(fn_bundle)
        except OSError, e:
            if e.errno != errno.ENOENT:
                _last_id += 1 # TODO: lock
                continue

            statinfo = None

        if statinfo and statinfo.st_size > kMaxBundleSize:
            _last_id += 1 # TODO: lock
            continue

        # 2
        fn_lock = '%s/%s' % (_lock_place, bid) # TODO: /place/.lock/sub-place/
        dir_lock = os.path.dirname(fn_lock)
        if not os.path.exists(dir_lock):
            os.makedirs(dir_lock)

        flock = FileLock(fn_lock, timeout=0.5)
        try:
            if not flock.try_acquire():
                _last_id += 1 # TODO: lock
                continue
        except:
            _last_id += 1 # TODO: lock
            continue

        assert flock.is_locked

        # 3
        if statinfo is None:
            fp = create(fn_bundle)
            fp.close()
            offset = kBundleHeaderSize
        else:
            statinfo = os.stat(fn_bundle)
            if statinfo.st_size > kMaxBundleSize:
                _last_id += 1 # TODO: lock
                continue
            offset = statinfo.st_size
        assert offset >= kBundleHeaderSize, offset
        return Writer(flock, filename=fn_bundle, bid=bid, offset=offset, length=length)

    return None

def release(w):
    pass
    # release w.lock

def new_bid(id, tm=None):
    """
    bid 在文件系统里为 08x，在 url 里被 sixty
    """
    if tm is None:
        tm = time.localtime()
    return '%s/%08x' % (time.strftime('%Y%m%d', tm), id)

def bundle_filename(bid, place):
    """
       >>> bundle_filename('1', 'p')
       'p/1'
       >>> bundle_filename('20110910/00000003', 'fmn04/x')
       'fmn04/x/20110910/00000003'
    """
    return os.path.join(place, bid)

def build_url(prefix, bid, offset, length, postfix):
  """
  构建最终用户能看到的 url
  
  a = (offset << 32) | length
  b = murmur(a)
  c = a ^ b
  fmn04/20100804/45/L/c_b.jpg

  >>> build_url("fmn04/large", '20110919/00000009', 0, 3, ".jpg")
  'fmn04/large/20110919/K/BFZf1G/BFZf1F.jpg'
  >>> build_url("fmn04/large", '20110919/00000009', 1024, 3, ".jpg")
  'fmn04/large/20110919/K/BkUlRx3O/Dp0WdL.jpg'
  >>> build_url(length=3123213, bid='20110930/00000190', prefix='fmn05/xxx/x', offset=12039123, postfix=".jpg")
  'fmn05/xxx/x/20110930/Gq/FH1PM2bygR/BzdJXz.jpg'
  """
  assert isinstance(prefix, str)
  assert isinstance(bid, str)
  assert type(offset) in [int, long]
  assert isinstance(length, int)
  assert isinstance(postfix, str)

  arr = bid.split('/')
  bs = '%s/%s' % (arr[0], sixty.ToSixty(int(arr[1], 16)))

  a = (offset << 32) | length
  b = murmur.MurmurHash2('%s/%s/%x.%s' % (prefix, bs, a, postfix))
  c = a ^ b

  # print '%x' % a, b, c
  return '%s/%s/%s/%s%s' % (prefix, bs, sixty.ToSixty(c), sixty.ToSixty(b), postfix)

def extract_url(url):
  """
  extract offset, length from url
  return dict
  fmn05/xxx/x/20110930/190/FH1PLnZ7ou/kU0pn.jpg
  \---------/  \-4     -3/   -2        -1    postfix
    prefix       ^^^^^^^^    c          b
                   bid

  >>> u = build_url("fmn04/large", '20110919/00000009', 1024, 3, ".jpg")
  >>> u
  'fmn04/large/20110919/K/BkUlRx3O/Dp0WdL.jpg'
  >>> extract_url(u)
  {'length': 3, 'bid': '20110919/00000009', 'prefix': 'fmn04/large', 'offset': 1024}
  >>> extract_url('fmn04/largF/20110919/K/BkUlRx3O/Dp0WdL.jpg')

  >>> extract_url('fmn04/large/20110918/K/BkUlRx3O/Dp0WdL.jpg')

  >>> extract_url('fmn04/large/20110919/L/BkUlRx3O/Dp0WdL.jpg')

  >>> extract_url('fmn04/large/20110919/K/EkUlRx3O/Dp0WdL.jpg')

  >>> extract_url('fmn04/large/20110919/K/BkUlRx3O/Ep0WdL.jpg')

  >>> extract_url('fmn04/large/20110919/K/BkUlRx3O/Dp0WdL.gif')

  >>> extract_url('test/rubbish/20110923/F/BkV10Cwe/E6Yzqo.jpg')
  {'length': 83283, 'bid': '20110923/00000005', 'prefix': 'test/rubbish', 'offset': 1024}
  """
  dot = url.rfind('.')
  postfix = url[dot:]
  arr = url[:dot].split('/')
  assert len(arr) >=5, url

  bid = '%s/%08x' %(arr[-4], sixty.FromSixty(arr[-3]))
  bs = '/'.join(arr[-4:-2])

  c = sixty.FromSixty(arr[-2]) 
  b = sixty.FromSixty(arr[-1]) # hash
  a = c ^ b
  # print '%x' % a, b, c

  prefix='/'.join(arr[:-4])

  b_check = murmur.MurmurHash2('%s/%s/%x.%s' % (prefix, bs, a, postfix))
  if b_check == b:
      return dict(prefix=prefix, 
                bid=bid,
                offset = int(a >> 32),
                length = int(a & 0xFFFFFFFF))
  return None

def create(filename):
    """
    建立bundle文件，并填充 header，返回 file object
    """
    folder = os.path.dirname(filename)

    if not os.path.exists(folder):
        os.makedirs(folder)

    bundleheader = "opi-corp.com file store\n1.0\n%s\n" % time.strftime('%Y-%m-%d %H:%M:%S')
    buf = bundleheader.ljust(kBundleHeaderSize, chr(0))

    fp = open(filename, "wb")
    fp.write(buf)
    assert fp.tell() == kBundleHeaderSize, fp.tell()
    return fp

def align1k(v):
  """
  对v按1024向上取整，
  >>> align1k(2048)
  2048
  >>> align1k(1024)
  1024
  >>> align1k(0)
  0
  >>> align1k(2049)
  3072
  >>> align1k(1)
  1024
  >>> align1k(88576)
  89088
  """
  a = v % 1024
  if a != 0:
    return v + 1024 - a
  else:
    return v

def walk(filename):
    """
    """
    # 先读取bundle头的信息
    fp = open(filename, 'rb')
    fp.seek(0)
    header = fp.read(kBundleHeaderSize)
  
    # TODO: check bundler header

    # 遍历bundle中的所有文件
    # TODO: 每次多读入一些 32k，性能稍好一些
    pos = kBundleHeaderSize
    while True:
        pos = align1k(pos)
        try:
            fp.seek(pos)
            fileheader = fp.read(kFileHeaderSize)
        except Exception, e:
            print e
            break

        if len(fileheader) < kFileHeaderSize:
            break

        fmt = 'IIII'
        magic, length, version, flag = struct.unpack(fmt, fileheader[:4*4])
        assert magic == kMAGIC
        assert version == 1
        assert flag == 1

        pos_zero = fileheader[16:].find(chr(0))
        assert pos_zero > 0 and pos_zero < kFileHeaderSize - 16
        url = fileheader[16:16 + pos_zero]
        # TODO: userdata
    
        content = fp.read(length - kFileHeaderSize)
        pos += length

        yield dict(flag=flag, url=url, content=content, userdata={})



if __name__ == '__main__':
    data = open('sample.jpg', 'rb').read()
    us = []

    w = allocate(len(data), 'p', tm=time.strptime('20110919', '%Y%m%d'))
    w.ensure_url(prefix='fmn04', postfix='.jpg')
    print 'write:', w.write(data), w.url
    us.append(w.url)
    del w

    w = allocate(len(data), 'p', tm=time.strptime('20110919', '%Y%m%d'))
    w.ensure_url(prefix='fmn05', postfix='.jpg')
    print 'write:', w.write(data), w.url
    us.append(w.url)
    del w

    for i, url in enumerate(us):
        assert data == read('p', url)

    import glob
    for bf in glob.glob('p/20110919/*'):
        for f in walk(bf):
            print f.keys(), len(f['content']), f['url']
