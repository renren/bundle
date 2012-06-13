# gyp --depth . -Dlibrary=static_library bundle.gyp -fmsvs -G msvs_version=2010
# gyp --depth . -Dlibrary=static_library bundle.gyp -fmake
# 
# drop -Gauto_regeneration=0, worked fine in ../gyp
# ls
# gyp src
# gyp --depth=. --toplevel-dir=. -Dlibrary=static_library gyp/bundle.gyp
{
  'conditions': [
    ['OS=="linux"', {
      'target_defaults': {
        'cflags': ['-fPIC', '-g', '-O2', '-Wall'], #, '-std=c++0x'],
        'defines': ['OS_POSIX', 'OS_LINUX', '_FILE_OFFSET_BITS=64'],
      },
    },],
    ['OS=="win"', {
      'target_defaults': {
        # 'cflags': ['-fPIC', '-g', '-O2',],
        'defines': ['WIN32', 'OS_WIN', 'NOMINMAX', 'UNICODE', '_UNICODE', 'WIN32_LEAN_AND_MEAN', '_WIN32_WINNT=0x0501'],
        'msvs_settings': {
          'VCLinkerTool': {'GenerateDebugInformation': 'true',
              'AdditionalLibraryDirectories': ['c:/boost_1_47_0/stage/lib'],
              'SubSystem': 1}, # Console
          'VCCLCompilerTool': {'DebugInformationFormat': '3',
              'AdditionalIncludeDirectories': ['c:/boost_1_47_0/'],
              'Optimization': 0}, # Disabled
        },
      },
    },],
  ],
  'targets': [
    {
      'target_name': 'bundle_unittest',
      'type': 'executable',
      'msvs_guid': '11384248-6F84-5DAE-8AB2-655600A90963',
      'dependencies': [
        'gtest.gyp:gtest_main',
        'bundle',
      ],
      'conditions':[
        ['OS=="linux"', {'libraries': ['-lboost_thread', '-lboost_system', '-lpthread'] }],
        ['OS=="win"', {'libraries': [] }],
      ],
      'sources': [
'../src/bundle/bundle_compile_test.cc',
'../src/bundle/filelock_test.cc',
'../src/bundle/sixty_test.cc',
'../src/bundle/bundle_test.cc',
'../src/bundle/bundle_bench.cc',
'../src/bundle/tmpl.h',
'../src/bundle/tmpl_test.cc',
      ],
      'include_dirs': [
        '../src',
        '../src/testing/gtest/include'
      ],
    },
    {
      'target_name': 'bundle_api_test',
      'type': 'executable',
      'msvs_guid': '11384258-7F84-5DAE-8AB2-655600A90963',
      'dependencies': [
        'gtest.gyp:gtest_main',
        'bundle',
      ],
      'conditions':[
        ['OS=="linux"', {'libraries': ['-lboost_thread', '-lboost_system', '-lpthread'] }],
        ['OS=="win"', {'libraries': [] }],
      ],
      'sources': [
'../src/bundle/bundle_api_test.cc',
      ],
      'include_dirs': [
        '../src',
        '../src/testing/gtest/include'
      ],
    },
    {
      'target_name': 'bundle',
      'type': 'static_library',
      'msvs_guid': 'B0FA2853-A0D3-44B8-BDE0-E8B89D372D16',
      'include_dirs': ['../src'],
      'dependencies': [
        'base3.gyp:base3',
      ],
      'sources': [
'../src/bundle/fslock.h',
'../src/bundle/bundle.h',
'../src/bundle/bundle.cc',
'../src/bundle/sixty.h',
'../src/bundle/murmurhash2.h',
'../src/bundle/murmurhash2.cc',
      ],
      #'export_dependent_settings': ['base3.gyp:base3'],
    },
    {
      'target_name': 'bundled',
      'type': 'executable',
      'msvs_guid': '2DEFD3FF-9903-41CD-AF2C-2249BE811651',
      'include_dirs': ['../src'],
      'dependencies': [
        'base3.gyp:base3',
        'cwf.gyp:cwfmain',
        'bundle',
      ],
      'conditions':[
        ['OS=="linux"', {'libraries': ['-lboost_thread', '-lboost_exception', '-lboost_system', '-lpthread'] }],
        ['OS=="win"', {'libraries': ['ws2_32.lib'] }],
      ],
      'sources': [
'../src/bundle/bundleaction.cc',
      ],
    },
    {
      'target_name': 'bundlec',
      'type': 'executable',
      'msvs_guid': '2DEFD3FF-9903-41CD-AF2C-2249BE811652',
      'include_dirs': ['../src'],
      'dependencies': [
        'base3.gyp:base3',
        'cwf.gyp:cwfmain',
        'bundle_with_mooseclient',
      ],
      'defines': ['USE_MOOSECLIENT=1'],
      'conditions':[
        ['OS=="linux"', {'libraries': ['-lboost_thread', '-lboost_exception', '-lboost_system', '-lpthread'] }],
        ['OS=="win"', {'libraries': ['ws2_32.lib'] }],
      ],
      'sources': [
'../src/bundle/bundleaction.cc',
      ],
    },
    {
      'target_name': 'bundle_write',
      'type': 'executable',
      'msvs_guid': '2DEFD3FF-9903-41CD-AF2C-2249BE811652',
      'include_dirs': ['../src'],
      'dependencies': [
        'base3.gyp:base3',
        'bundle',
      ],
      'conditions':[
        ['OS=="linux"', {'libraries': ['-lboost_system', '-lboost_thread', '-lpthread'] }],
        ['OS=="win"', {'libraries': ['ws2_32.lib'] }],
      ],
      'sources': [
'../src/bundle/bundlewrite.cc',
      ],
    },
    {
      'target_name': 'bundle_with_mooseclient',
      'type': 'shared_library',
      'msvs_guid': 'B0FA2853-A0D3-44B8-BDE1-E8B89D372D17',
      'include_dirs': ['../src'],
      'dependencies': ['mooseclient.gyp:mooseclient'],
      'conditions':[
        ['OS=="linux"', {'libraries': ['-lboost_thread', '-lboost_system', '-lpthread', '-lrt'] }],
        ['OS=="win"', {'libraries': [] }],
      ],
      'sources': [
'../src/bundle/fslock.h',
'../src/bundle/bundle.h',
'../src/bundle/bundle.cc',
'../src/bundle/sixty.h',
'../src/bundle/murmurhash2.h',
'../src/bundle/murmurhash2.cc',
      ],
      'defines': ['USE_MOOSECLIENT=1'],
    },
    {
      'target_name': 'bundle_with_mooseclient_unittest',
      'type': 'executable',
      'msvs_guid': '11384248-6F84-5DAF-9AB2-655600A90964',
      'dependencies': [
        'gtest.gyp:gtest',
        'bundle_with_mooseclient',
      ],
      'defines': ['USE_MOOSECLIENT=1'],
      'conditions':[
        ['OS=="linux"', {'libraries': ['-lboost_thread', '-lboost_system', '-lpthread'] }],
        ['OS=="win"', {'libraries': [] }],
      ],
      'sources': [
'../src/bundle/bundle_test.cc',
'../src/bundle/bundle_bench.cc',
'../src/bundle/filelock_test.cc',
      ],
      'include_dirs': [
        '../src',
        '../src/testing/gtest/include'
      ],
    },
  ],
}
