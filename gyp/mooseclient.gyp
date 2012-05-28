# gyp --depth . -Dlibrary=static_library mooseclient.gyp -fmsvs -G msvs_version=2010
# gyp --depth . -Dlibrary=static_library mooseclient.gyp -fmake
# drop -Gauto_regeneration=0, worked fine in ../gyp
# ls
# gyp src
# gyp --depth=. --toplevel-dir=. -Dlibrary=static_library gyp/mooseclient.gyp
{
  'conditions': [
    ['OS=="linux"', {
      'target_defaults': {
        'cflags': ['-fPIC', '-g', '-O0', '-Wall','-std=c++0x'],
        'defines': ['OS_LINUX', '_FILE_OFFSET_BITS=64'],
      },
    },],
    ['OS=="win"', {
      'target_defaults': {
        # 'cflags': ['-fPIC', '-g', '-O2',],
        'defines': ['WIN32', 'OS_WIN', 'NOMINMAX', 'UNICODE', '_UNICODE', 'WIN32_LEAN_AND_MEAN', '_WIN32_WINNT=0x0501'],
        'include_dirs': ['c:/boost_1_47_0'],
        'msvs_settings': {
          'VCLinkerTool': {'GenerateDebugInformation': 'true',},
          'VCCLCompilerTool': {
             'DebugInformationFormat': '3',
             'Optimization': '0'
          },
        },
      },
    },],
  ],
  'targets': [
    {
      'target_name': 'mooseclient_bench',
      'type': 'executable',
      'msvs_guid': '2DEFD400-9904-41CD-A03C-3349BE811651',
      'include_dirs': ['../src'],
      'dependencies': ['mooseclient'],
      'defines': ['PROTO_BASE=0'],
      'conditions':[
        ['OS=="linux"', {'libraries': ['-lrt', '-lpthread'] }],
        ['OS=="win"', {
          'libraries': ['ws2_32.lib'],
          'dependencies': ['base3.gyp:base3'],
        }],
      ],
      'sources': [
'../src/mooseclient/api_test.cc',
      ],
    },
    {
      'target_name': 'api_c_test',
      'type': 'executable',
      'msvs_guid': '2DEFD411-AA04-42CD-A03C-3349BE811652',
      'include_dirs': [
        '../src',
        '../src/testing/gtest/include'
      ],
      'dependencies': [
        'mooseclient', 
        'gtest.gyp:gtest_main',
      ],
      'conditions':[
        ['OS=="linux"', {'libraries': ['-lrt', '-lpthread', '-ldl'] }],
        ['OS=="win"', {
          'libraries': ['ws2_32.lib'],
          'dependencies': ['base3.gyp:base3'],
        }],
      ],
      'sources': [
'../src/mooseclient/api_c_test.cc',
      ],
    },
    {
      'target_name': 'preload_test',
      'type': 'executable',
      'msvs_guid': '2DEFD400-9904-42CD-A03C-3349BE811652',
      'include_dirs': [
        '../src',
        '../src/testing/gtest/include'
      ],
      'dependencies': [
        'preloadfs', 
        'gtest.gyp:gtest_main',
      ],
      'conditions':[
        ['OS=="linux"', {'libraries': ['-lrt', '-lpthread', '-ldl'] }],
        ['OS=="win"', {
          'libraries': ['ws2_32.lib'],
          'dependencies': ['base3.gyp:base3'],
        }],
      ],
      'sources': [
'../src/mooseclient/preload_test.cc',
      ],
    },
    {
      'target_name': 'dynaload_test',
      'type': 'executable',
      'msvs_guid': '2DEFD400-AA15-42CD-A03C-3349BE811652',
      'include_dirs': [
        '../src',
        '../src/testing/gtest/include'
      ],
      'dependencies': [
        'gtest.gyp:gtest_main',
      ],
      'conditions':[
        ['OS=="linux"', {'libraries': ['-lrt', '-lpthread', '-ldl'] }],
        ['OS=="win"', {
          'libraries': ['ws2_32.lib'],
          'dependencies': ['base3.gyp:base3'],
        }],
      ],
      'sources': [
'../src/mooseclient/dynaload_test.cc',
      ],
    },
    {
      'target_name': 'preloadfs',
      'type': 'shared_library',
      'msvs_guid': '2DFFD401-A904-42CD-A03C-3349BE911654',
      'include_dirs': ['../src'],
      'dependencies': ['base3.gyp:base3', 'mooseclient'],
      'defines': ['PROTO_BASE=0'],
      'conditions':[
        ['OS=="linux"', {'libraries': ['-lboost_thread', '-lrt', '-lpthread', '-ldl'] }],
        ['OS=="win"', {
          'libraries': ['ws2_32.lib'],
        }],
      ],
      'sources': [
'../src/mooseclient/preload.h',
'../src/mooseclient/preload.cc',
      ],
    },
    {
      'target_name': 'mooseclient',
      'type': 'static_library',
      'msvs_guid': '2EF0D400-9904-41CD-A03C-3349BE811651',
      'include_dirs': ['../src'],
      'dependencies': ['base3.gyp:base3'],
      'defines': ['PROTO_BASE=0'],
      'conditions':[
        ['OS=="linux"', {'libraries': ['-lrt', '-lpthread'] }],
        ['OS=="win"', {
          'libraries': ['ws2_32.lib'],
          'dependencies': [],
        }],
      ],
      'sources': [
'../src/mooseclient/moose.h',
'../src/mooseclient/moose.cc',
'../src/mooseclient/sockutil.h',
'../src/mooseclient/sockutil.cc',
'../src/moosefs/datapack.h',
'../src/moosefs/datapack_win.h',
'../src/moosefs/MFSCommunication.h',
'../src/mooseclient/moose_c.cc',
'../src/mooseclient/moose_c.h',
      ],
    }
  ],
}
