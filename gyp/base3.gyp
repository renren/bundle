# gyp --depth . -Dlibrary=static_library base3.gyp -fmsvs -G msvs_version=2008e
# gyp --depth=. --toplevel-dir=. -Dlibrary=static_library gyp/base3.gyp
{
  'conditions': [
    ['OS=="linux"', {
      'target_defaults': {
        'cflags': ['-fPIC', '-g', '-O2', '-pthread'],
         # USE_PROFILER
        'defines': ['OS_LINUX','BASE_DISABLE_POOL', 'NO_TCMALLOC'],
      },
    },],
    ['OS=="win"', {
      'target_defaults': {
        # 'cflags': ['-fPIC', '-g', '-O2',],
        'defines': ['OS_WIN', 'ARCH_CPU_X86_FAMILY', 'NOMINMAX', 'UNICODE', '_UNICODE', 'WIN32_LEAN_AND_MEAN', '_WIN32_WINNT=0x0501', 'BASE_DISABLE_POOL'],
        'include_dirs': ['c:/boost_1_45_0'],
        'msvs_settings': {
          'VCLinkerTool': {'GenerateDebugInformation': 'true',},
          'VCCLCompilerTool': {'DebugInformationFormat': '3',},
        },
      },
    },],
  ],
  'targets': [
    {
      'target_name': 'base_naketest',
      'type': 'executable',
      'dependencies': [
        'base3',
        'gtest.gyp:gtest_main',
      ],
      'conditions':[
        ['OS=="linux"', {'libraries': ['-lboost_system', '-lboost_thread', '-lpthread', '-ltcmalloc'] }],
      ],
      'include_dirs': [
        '../src',
        # '../src/testing/gtest/include',
      ],
      'sources': [
        '../src/base3/logging_test.cc',
      ],
    },
    {
      'target_name': 'base_unittest',
      'type': 'executable',
      'dependencies': [
        'base3',
        'gtest.gyp:gtest_main',
      ],
      'conditions':[
        ['OS=="linux"', {'libraries': ['-lboost_system', '-lboost_thread', '-lpthread' ] }],
      ],
      'sources': [
#'../src/base3/asyncall2_test.cc',
#'../src/base3/asyncall_test.cc',
'../src/base3/atomicops_test.cc',
#'../src/base3/baseasync_test.cc',
'../src/base3/boost_test.cc',
'../src/base3/cache_test.cc',
'../src/base3/common_test.cc',
'../src/base3/directstream_test.cc',
'../src/base3/doobs_hash_test.cc',
'../src/base3/gtest_prod_util.h',
'../src/base3/hash_test.cc',
'../src/base3/jdbc_test.cc',
'../src/base3/logging_test.cc',
'../src/base3/logrotate_test.cc',
'../src/base3/md5_test.cc',
'../src/base3/messagequeue_test.cc',
'../src/base3/mkdirs_test.cc',
'../src/base3/pathops_test.cc',
'../src/base3/pcount_test.cc',
'../src/base3/ptime_test.cc',
'../src/base3/rwlock_test.cc',
'../src/base3/stringencode_test.cc',
'../src/base3/string_piece_unittest.cc',
'../src/base3/ref_counted_unittest.cc',
'../src/base3/tcmalloc_test.cc',
#'../src/base3/threadpool_test.cc',
'../src/base3/types_test.cc',
      ],
      'include_dirs': [
        '../src',
        #'../src/testing/gtest/include',
      ],
    },
    {
      'target_name': 'base3',
      'type': 'static_library',
      'msvs_guid': '9301A569-5D2B-4D11-9332-B1E30AEACB8D',
      'include_dirs': ['../src'],
      'dependencies': [],
      'sources': [
# '../src/base3/asyncall.cc',
# '../src/base3/asyncall.h',
'../src/base3/atomicops.h',
'../src/base3/atomicops_internals_arm_gcc.h',
'../src/base3/atomicops_internals_x86_gcc.cc',
'../src/base3/atomicops_internals_x86_gcc.h',
'../src/base3/atomicops_internals_x86_msvc.h',
'../src/base3/atomic_ref_count.h',
# '../src/base3/baseasync.cc',
# '../src/base3/baseasync.h',
'../src/base3/basictypes.h',
'../src/base3/build_config.h',
'../src/base3/cache.h',
'../src/base3/circular_count.h',
#'../src/base3/cpu.h',
#'../src/base3/cpu.cc',
'../src/base3/common.cc',
'../src/base3/common.h',
'../src/base3/compiler_specific.h',
'../src/base3/consistenthash.h',
'../src/base3/directstream.h',
'../src/base3/doobs_hash.cc',
'../src/base3/doobs_hash.h',
'../src/base3/eintr_wrapper.h',
'../src/base3/escape.h',
'../src/base3/file_descriptor_posix.h',
#'../src/base3/file_descriptor_shuffle.cc',
#'../src/base3/file_descriptor_shuffle.h',
'../src/base3/getopt.c',
'../src/base3/getopt_.h',
'../src/base3/globalinit.h',
'../src/base3/hash.h',
'../src/base3/hashmap.h',
'../src/base3/jdbcurl.cc',
'../src/base3/jdbcurl.h',
# '../src/base3/lock.cc',
# '../src/base3/lock.h',
# '../src/base3/lock_impl.h',
# '../src/base3/lock_impl_posix.cc',
# '../src/base3/lock_impl_win.cc',
'../src/base3/logging.cc',
'../src/base3/logging.h',
'../src/base3/logrotate.h',
'../src/base3/md5.cc',
'../src/base3/md5.h',
'../src/base3/messagequeue.cc',
'../src/base3/messagequeue.h',
'../src/base3/metrics/field_trial.cc',
'../src/base3/metrics/field_trial.h',
'../src/base3/metrics/histogram.cc',
'../src/base3/metrics/histogram.h',
'../src/base3/metrics/nacl_histogram.cc',
'../src/base3/metrics/nacl_histogram.h',
'../src/base3/metrics/stats_counters.cc',
'../src/base3/metrics/stats_counters.h',
'../src/base3/metrics/stats_table.cc',
'../src/base3/metrics/stats_table.h',
'../src/base3/mkdirs.cc',
'../src/base3/mkdirs.h',
'../src/base3/network.cc',
'../src/base3/network.h',
'../src/base3/once.cc',
'../src/base3/once.h',
'../src/base3/pathops.cc',
'../src/base3/pathops.h',
# '../src/base3/pcount.cc',
# '../src/base3/pcount.h',
'../src/base3/platform_thread.h',
'../src/base3/platform_thread_posix.cc',
'../src/base3/platform_thread_win.cc',
'../src/base3/port.h',
# '../src/base3/process.h',
# '../src/base3/process_linux.cc',
# '../src/base3/process_posix.cc',
# '../src/base3/process_util.cc',
# '../src/base3/process_util.h',
# '../src/base3/process_util_linux.cc',
# '../src/base3/process_util_posix.cc',
# '../src/base3/process_util_win.cc',
# '../src/base3/process_win.cc',
# '../src/base3/ptime.h',
# '../src/base3/rand_util.cc',
# '../src/base3/rand_util_c.h',
# '../src/base3/rand_util.h',
# '../src/base3/rand_util_posix.cc',
# '../src/base3/rand_util_win.cc',
'../src/base3/README.txt',
'../src/base3/ref_counted.cc',
'../src/base3/ref_counted.h',
'../src/base3/ref_counted_memory.cc',
'../src/base3/ref_counted_memory.h',
'../src/base3/rwlock.h',
'../src/base3/scoped_ptr.h',
# '../src/base3/shared_memory.h',
# '../src/base3/shared_memory_posix.cc',
# '../src/base3/shared_memory_win.cc',
'../src/base3/signals.cc',
'../src/base3/signals.h',
'../src/base3/startuplist.cc',
'../src/base3/startuplist.h',
'../src/base3/stringdigest.cc',
'../src/base3/stringdigest.h',
'../src/base3/stringencode.cc',
'../src/base3/stringencode.h',
'../src/base3/string_number_conversions.cc',
'../src/base3/string_number_conversions.h',
'../src/base3/string_piece.cc',
'../src/base3/string_piece.h',
'../src/base3/stringprintf.cc',
'../src/base3/stringprintf.h',
'../src/base3/string_split.cc',
'../src/base3/string_split.h',
'../src/base3/string_tokenizer.h',
'../src/base3/string_util.cc',
'../src/base3/string_util.h',
'../src/base3/string_util_posix.h',
'../src/base3/string_util_win.h',
'../src/base3/third_party/dmg_fp/dmg_fp.h',
'../src/base3/third_party/dmg_fp/dtoa.cc',
'../src/base3/third_party/dmg_fp/gcc_64_bit.patch',
'../src/base3/third_party/dmg_fp/gcc_warnings.patch',
'../src/base3/third_party/dmg_fp/g_fmt.cc',
'../src/base3/third_party/dmg_fp/LICENSE',
'../src/base3/third_party/dmg_fp/README.chromium',
'../src/base3/third_party/dynamic_annotations/dynamic_annotations.c',
'../src/base3/third_party/dynamic_annotations/dynamic_annotations.gyp',
'../src/base3/third_party/dynamic_annotations/dynamic_annotations.h',
'../src/base3/third_party/dynamic_annotations/LICENSE',
'../src/base3/third_party/dynamic_annotations/README.chromium',
'../src/base3/third_party/valgrind/LICENSE',
'../src/base3/third_party/valgrind/README.chromium',
'../src/base3/third_party/valgrind/valgrind.h',
# '../src/base3/thread.cc',
# '../src/base3/thread_checker.cc',
# '../src/base3/thread_checker.h',
# '../src/base3/thread_collision_warner.cc',
# '../src/base3/thread_collision_warner.h',
# '../src/base3/_thread.h',
# '../src/base3/thread.h',
# '../src/base3/thread_local.h',
# '../src/base3/thread_local_posix.cc',
# '../src/base3/thread_local_storage.h',
# '../src/base3/thread_local_storage_posix.cc',
# '../src/base3/thread_local_storage_win.cc',
# '../src/base3/thread_local_win.cc',
# '../src/base3/thread_restrictions.cc',
# '../src/base3/thread_restrictions.h',
'../src/base3/time.cc',
'../src/base3/time.h',
'../src/base3/time_posix.cc',
'../src/base3/time_win.cc',
'../src/base3/charmask.h',
'../src/base3/url.cc',
'../src/base3/url.h',
      ],
      # TODO: better way
      'direct_dependent_settings': {
        'include_dirs': ['../src/testing/gtest/include'],
      },
      'conditions': [
        ['OS!="mac"', {'sources/': [['exclude', '_mac\\.(cc|mm?)$']]}],
        ['OS!="win"', {'sources/': [['exclude', '_win\\.cc$']]
          }, {  # else: OS=="win"
            'sources/': [['exclude', '_posix\\.cc$']]
        }],
        ['OS=="linux"', {
          'direct_dependent_settings': {
            'libraries': ['-lrt', '-lpthread'], #'-ltcmalloc_and_profiler',
          },
        }],
      ],
    },
  ],
}
