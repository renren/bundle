# gyp --depth . -Dlibrary=static_library mfs.gyp -fmsvs -G msvs_version=2010
{
  'conditions': [
    ['OS=="linux"', {
      'target_defaults': {
        'cflags': ['-fPIC', '-g', '-O2',],
        'defines': ['OS_LINUX'],
      },
    },],
    ['OS=="win"', {
      'target_defaults': {
        # 'cflags': ['-fPIC', '-g', '-O2',],
        'defines': ['WIN32', 'OS_WIN', 'NOMINMAX', 'UNICODE', '_UNICODE', 'WIN32_LEAN_AND_MEAN', '_WIN32_WINNT=0x0501'],
        'msvs_settings': {
          'VCLinkerTool': {'GenerateDebugInformation': 'true',},
          'VCCLCompilerTool': {'DebugInformationFormat': '3',},
        },
		'include_dirs': ['%(INCLUDE)'],
      },
    },],
  ],
  'targets': [
    {
      'target_name': 'cwf_unittest',
      'type': 'executable',
      'msvs_guid': '11384247-5F84-4DAE-8AB2-655600A90963',
      'dependencies': [
        'cwf',
        'libfcgi',
        'base3.gyp:base3',
        #'google-ctemplate.gyp:ctemplate',
        '../src/testing/gtest.gyp:gtest_main',
      ],
      'conditions':[
        ['OS=="linux"', {'libraries': ['-lboost_system', '-lboost_thread', '-lpthread'] }],
        ['OS=="win"', {'libraries': ['ws2_32.lib'] }],
      ],
      'sources': [
        '../src/cwf/stream_test.cc',
      ],
      'include_dirs': [
        '../src',
      ],
    },
    {
      'target_name': 'cwf',
      'type': 'static_library',
      'msvs_guid': 'B0FA2852-A0D3-44B8-BDE0-E8B89D372D05',
      'include_dirs': ['../src'],
      'dependencies': [
        'base3.gyp:base3',
        'libfcgi',
      ],
      'sources': [
'../src/cwf/404.tpl',
'../src/cwf/action.h',
'../src/cwf/action.cc',
# '../src/cwf/config_parse.h',
'../src/cwf/connect.cc',
'../src/cwf/connect.h',
'../src/cwf/cookie.cc',
'../src/cwf/cookie.h',
'../src/cwf/cwf.h',
# '../src/cwf/doc/fcgi-spec.html',
# '../src/cwf/doc/resource.txt',
# '../src/cwf/dynalib.cc',
# '../src/cwf/dynalib.h',
'../src/cwf/foo.tpl',
'../src/cwf/frame.cc',
'../src/cwf/frame.h',
'../src/cwf/http.cc',
'../src/cwf/http.h',
'../src/cwf/pattern.h',
'../src/cwf/setting',
'../src/cwf/site.h',
'../src/cwf/stream.cc',
'../src/cwf/stream.h',
# '../src/cwf/task.h',
# '../src/cwf/tplaction.cc',
# '../src/cwf/tplaction.h',
      ],
      'export_dependent_settings': ['base3.gyp:base3'],
      'conditions': [
        ['OS == "win"', {
          'direct_dependent_settings': {
              'libraries': ['ws2_32.lib'],
            },
          },
        ],
        ['OS == "linux"', {
          'direct_dependent_settings': {
              #'ldflags': ['<(DEPTH)'],
            },
          },
        ],
      ],
    },
    {
      'target_name': 'cwfd',
      'type': 'executable',
      'msvs_guid': '2DEFD3EE-88E3-41CD-AF2C-2249BE811651',
      'include_dirs': ['../src'],
      'dependencies': [
        'cwf',
        'cwfmain',
        #'google-ctemplate.gyp:ctemplate', 
      ],
      #'export_dependent_settings': ['google-ctemplate.gyp:ctemplate'],
      'conditions':[
        ['OS=="linux"', {'libraries': ['-lboost_system', '-lboost_thread', '-lpthread'] }],
        ['OS=="win"', {'libraries': ['ws2_32.lib'] }],
      ],
      'sources': [
'../src/cwf/404.tpl',
'../src/cwf/tplaction.cc',
'../src/cwf/tplaction.h',
      ],
    },
    {
      'target_name': 'cwfmain',
      'type': 'static_library',
      'msvs_guid': '0829F556-C016-4F0A-8C1D-A094AD174534',
      'include_dirs': ['../src'],
      'dependencies': ['cwf', 'libfcgi'],
      'export_dependent_settings': ['cwf'],
      'sources': [
'../src/cwf/cwfmain.cc',
      ],
    },
    {
      'target_name': 'libfcgi',
      'type': 'static_library',
      'msvs_guid': '7D8110AD-6AA4-4A68-AACA-3DC84541C6A0',
      # 'include_dirs': ['..'],
      'sources': [
'../src/3rdparty/libfcgi/fastcgi.h',
'../src/3rdparty/libfcgi/fcgiapp.c',
'../src/3rdparty/libfcgi/fcgiapp.h',
'../src/3rdparty/libfcgi/fcgi_config.h',
'../src/3rdparty/libfcgi/fcgi_config_x86.h',
'../src/3rdparty/libfcgi/fcgimisc.h',
'../src/3rdparty/libfcgi/fcgio.cpp',
'../src/3rdparty/libfcgi/fcgio.h',
'../src/3rdparty/libfcgi/fcgios.h',
'../src/3rdparty/libfcgi/fcgi_stdio.c',
'../src/3rdparty/libfcgi/fcgi_stdio.h',
'../src/3rdparty/libfcgi/os_unix.c',
'../src/3rdparty/libfcgi/os_win32.c',
      ],
      'conditions': [
        ['OS == "linux"', {
          'sources!': [
'../src/3rdparty/libfcgi/os_win32.c',
            ],}],
        ['OS == "win"', {
          'sources!': [
'../src/3rdparty/libfcgi/os_unix.c',
            ],}]
      ],
    },
  ],
}
