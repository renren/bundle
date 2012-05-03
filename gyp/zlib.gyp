{
  'conditions': [
    ['OS=="linux"', {
      'target_defaults': {
        'cflags': ['-fPIC', '-g', '-O3',],
        #'defines': ['OS_LINUX'],
      },
    },],
    ['OS=="win"', {
      'target_defaults': {
        # 'cflags': ['-fPIC', '-g', '-O2',],
        #'defines': ['OS_WIN', 'WIN32', 'NOMINMAX', 'UNICODE', '_UNICODE', 'WIN32_LEAN_AND_MEAN', '_WIN32_WINNT=0x0501'],
      },
    },],
  ],
  'targets': [
  {
    'target_name': 'zlib',
    'type': 'static_library',
    'include_dirs': ['../src/3rdparty/zlib-stable'],
    'msvs_guid': '8423AF0D-4B88-4EBF-94E1-E4D00D00E21C',
    'sources': [
      '../src/3rdparty/zlib-stable/adler32.c',
      '../src/3rdparty/zlib-stable/compress.c',
      '../src/3rdparty/zlib-stable/crc32.c',
      '../src/3rdparty/zlib-stable/crc32.h',
      '../src/3rdparty/zlib-stable/deflate.c',
      '../src/3rdparty/zlib-stable/deflate.h',
      '../src/3rdparty/zlib-stable/gzlib.c',
      '../src/3rdparty/zlib-stable/gzclose.c',
      '../src/3rdparty/zlib-stable/gzread.c',
      '../src/3rdparty/zlib-stable/gzwrite.c',
      '../src/3rdparty/zlib-stable/infback.c',
      '../src/3rdparty/zlib-stable/inffast.c',
      '../src/3rdparty/zlib-stable/inffast.h',
      '../src/3rdparty/zlib-stable/inffixed.h',
      '../src/3rdparty/zlib-stable/inflate.c',
      '../src/3rdparty/zlib-stable/inflate.h',
      '../src/3rdparty/zlib-stable/inftrees.c',
      '../src/3rdparty/zlib-stable/inftrees.h',
#      '../src/3rdparty/zlib-stable/mozzconf.h',
      '../src/3rdparty/zlib-stable/trees.c',
      '../src/3rdparty/zlib-stable/trees.h',
      '../src/3rdparty/zlib-stable/uncompr.c',
      '../src/3rdparty/zlib-stable/zconf.h',
      '../src/3rdparty/zlib-stable/zlib.h',
      '../src/3rdparty/zlib-stable/zutil.c',
      '../src/3rdparty/zlib-stable/zutil.h',
    ],
    'direct_dependent_settings': {
      'include_dirs': ['../src/3rdparty/zlib-stable'],
    },
  },]
}