# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables' : {'clang' : 0},
  'targets': [
    {
      'target_name': 'gtest',
      'type': '<(library)',
      'msvs_guid': 'BFE8E2A7-3B3B-43B0-A994-3058B852DB8B',
      'sources': [
        '../src/testing/gtest/include/gtest/gtest-death-test.h',
        '../src/testing/gtest/include/gtest/gtest-message.h',
        '../src/testing/gtest/include/gtest/gtest-param-test.h',
        '../src/testing/gtest/include/gtest/gtest-printers.h',
        '../src/testing/gtest/include/gtest/gtest-spi.h',
        '../src/testing/gtest/include/gtest/gtest-test-part.h',
        '../src/testing/gtest/include/gtest/gtest-typed-test.h',
        '../src/testing/gtest/include/gtest/gtest.h',
        '../src/testing/gtest/include/gtest/gtest_pred_impl.h',
        '../src/testing/gtest/include/gtest/gtest_prod.h',
        '../src/testing/gtest/include/gtest/internal/gtest-death-test-internal.h',
        '../src/testing/gtest/include/gtest/internal/gtest-filepath.h',
        '../src/testing/gtest/include/gtest/internal/gtest-internal.h',
        '../src/testing/gtest/include/gtest/internal/gtest-linked_ptr.h',
        '../src/testing/gtest/include/gtest/internal/gtest-param-util-generated.h',
        '../src/testing/gtest/include/gtest/internal/gtest-param-util.h',
        '../src/testing/gtest/include/gtest/internal/gtest-port.h',
        '../src/testing/gtest/include/gtest/internal/gtest-string.h',
        '../src/testing/gtest/include/gtest/internal/gtest-tuple.h',
        '../src/testing/gtest/include/gtest/internal/gtest-type-util.h',
        '../src/testing/gtest/src/gtest-all.cc',
        '../src/testing/gtest/src/gtest-death-test.cc',
        '../src/testing/gtest/src/gtest-filepath.cc',
        '../src/testing/gtest/src/gtest-internal-inl.h',
        '../src/testing/gtest/src/gtest-port.cc',
        '../src/testing/gtest/src/gtest-printers.cc',
        '../src/testing/gtest/src/gtest-test-part.cc',
        '../src/testing/gtest/src/gtest-typed-test.cc',
        '../src/testing/gtest/src/gtest.cc',
        '../src/testing/multiprocess_func_list.cc',
        '../src/testing/multiprocess_func_list.h',
        '../src/testing/platform_test.h',
      ],
      'sources!': [
        '../src/testing/gtest/src/gtest-all.cc',  # Not needed by our build.
      ],
      'include_dirs': [
        '../src/testing/gtest',
        '../src/testing/gtest/include',
      ],
      'conditions': [
        ['OS == "mac"', {
          'sources': [
            'gtest_mac.h',
            'gtest_mac.mm',
            'platform_test_mac.mm'
          ],
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/Foundation.framework',
            ],
          },
        }],
        ['OS == "mac" or OS == "linux" or OS == "freebsd" or OS == "openbsd"', {
          'defines': [
            # gtest isn't able to figure out when RTTI is disabled for gcc
            # versions older than 4.3.2, and assumes it's enabled.  Our Mac
            # and Linux builds disable RTTI, and cannot guarantee that the
            # compiler will be 4.3.2. or newer.  The Mac, for example, uses
            # 4.2.1 as that is the latest available on that platform.  gtest
            # must be instructed that RTTI is disabled here, and for any
            # direct dependents that might include gtest headers.
            'GTEST_HAS_RTTI=0',
          ],
          'direct_dependent_settings': {
            'defines': [
              'GTEST_HAS_RTTI=0',
            ],
          },
        }],
        ['clang==1', {
          # We want gtest features that use tr1::tuple, but clang currently
          # doesn't support the variadic templates used by libstdc++'s
          # implementation.  gtest supports this scenario by providing its
          # own implementation but we must opt in to it.
          'defines': [
            'GTEST_USE_OWN_TR1_TUPLE=1',
          ],
          'direct_dependent_settings': {
            'defines': [
              'GTEST_USE_OWN_TR1_TUPLE=1',
            ],
          },
        }],
      ],
      'direct_dependent_settings': {
        'defines': [
          'UNIT_TEST',
        ],
        'include_dirs': [
          #'gtest/include',  # So that gtest headers can find themselves.
          '../src/testing/gtest',
          '../src/testing/gtest/include',
        ],
        'target_conditions': [
          ['_type=="executable"', {
            'test': 1,
            'conditions': [
              ['OS=="mac"', {
                'run_as': {
                  'action????': ['${BUILT_PRODUCTS_DIR}/${PRODUCT_NAME}'],
                },
              }],
              ['OS=="win"', {
                'run_as': {
                  'action????': ['$(TargetPath)', '--gtest_print_time'],
                },
              }],
            ],
          }],
        ],
        'msvs_disabled_warnings': [4800],
      },
    },
    {
      'target_name': 'gtest_main',
      'type': '<(library)',
      'dependencies': [
        'gtest',
      ],
      'sources': [
        '../src/testing/gtest/src/gtest_main.cc',
      ],
    },
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
