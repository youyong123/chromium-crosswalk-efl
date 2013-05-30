# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'pull_in_copy_TestNetscapePlugIn',
      'type': 'none',
      'dependencies': [
        '../third_party/WebKit/Tools/DumpRenderTree/DumpRenderTree.gyp/DumpRenderTree.gyp:copy_TestNetscapePlugIn'
      ],
    },
    {
      # TODO(tony): This should be moved to webkit_glue.gypi or
      # webkit_tests.gypi and named something like test_mock_plugin_list.
      'target_name': 'test_shell_test_support',
      'type': 'static_library',
      'dependencies': [
        '<(DEPTH)/webkit/support/webkit_support.gyp:glue'
      ],
      'sources': [
        '../../plugins/npapi/mock_plugin_list.cc',
        '../../plugins/npapi/mock_plugin_list.h',
      ]
    },
  ],
  'conditions': [
    # Currently test_shell compiles only on Windows, Mac, and Gtk.
    ['OS=="win" or OS=="mac" or toolkit_uses_gtk==1', {
      'targets': [
        {
          # TODO(darin): Delete this dummy target once the build masters stop
          # trying to build it.
          'target_name': 'test_shell',
          'type': 'static_library',
          'sources': [
            'test_shell_dummy.cc',
          ],
        },
      ], 
    }],
    ['OS!="android" and OS!="ios"', {
      # npapi test plugin doesn't build on android or ios
      'targets': [
        {
          'target_name': 'npapi_test_common',
          'type': 'static_library',
          'dependencies': [
            '<(DEPTH)/base/base.gyp:base',
            '<(DEPTH)/third_party/npapi/npapi.gyp:npapi',
          ],
          'sources': [
            '../../plugins/npapi/test/npapi_constants.cc',
            '../../plugins/npapi/test/npapi_constants.h',
            '../../plugins/npapi/test/plugin_client.cc',
            '../../plugins/npapi/test/plugin_client.h',
            '../../plugins/npapi/test/plugin_test.cc',
            '../../plugins/npapi/test/plugin_test.h',
            '../../plugins/npapi/test/plugin_test_factory.h',
          ],
          'export_dependent_settings': [
            '<(DEPTH)/base/base.gyp:base',
          ],
        },
        {
          'target_name': 'npapi_test_plugin',
          'type': 'loadable_module',
          'variables': {
            'chromium_code': 1,
          },
          'mac_bundle': 1,
          'dependencies': [
            '<(DEPTH)/third_party/icu/icu.gyp:icuuc',
            'npapi_test_common',
          ],
          'sources': [
            '../../plugins/npapi/test/npapi_test.cc',
            '../../plugins/npapi/test/npapi_test.def',
            '../../plugins/npapi/test/npapi_test.rc',
            '../../plugins/npapi/test/plugin_arguments_test.cc',
            '../../plugins/npapi/test/plugin_arguments_test.h',
            '../../plugins/npapi/test/plugin_create_instance_in_paint.cc',
            '../../plugins/npapi/test/plugin_create_instance_in_paint.h',
            '../../plugins/npapi/test/plugin_delete_plugin_in_deallocate_test.cc',
            '../../plugins/npapi/test/plugin_delete_plugin_in_deallocate_test.h',
            '../../plugins/npapi/test/plugin_delete_plugin_in_stream_test.cc',
            '../../plugins/npapi/test/plugin_delete_plugin_in_stream_test.h',
            '../../plugins/npapi/test/plugin_execute_stream_javascript.cc',
            '../../plugins/npapi/test/plugin_execute_stream_javascript.h',
            '../../plugins/npapi/test/plugin_get_javascript_url_test.cc',
            '../../plugins/npapi/test/plugin_get_javascript_url_test.h',
            '../../plugins/npapi/test/plugin_get_javascript_url2_test.cc',
            '../../plugins/npapi/test/plugin_get_javascript_url2_test.h',
            '../../plugins/npapi/test/plugin_geturl_test.cc',
            '../../plugins/npapi/test/plugin_geturl_test.h',
            '../../plugins/npapi/test/plugin_javascript_open_popup.cc',
            '../../plugins/npapi/test/plugin_javascript_open_popup.h',
            '../../plugins/npapi/test/plugin_new_fails_test.cc',
            '../../plugins/npapi/test/plugin_new_fails_test.h',
            '../../plugins/npapi/test/plugin_npobject_identity_test.cc',
            '../../plugins/npapi/test/plugin_npobject_identity_test.h',
            '../../plugins/npapi/test/plugin_npobject_lifetime_test.cc',
            '../../plugins/npapi/test/plugin_npobject_lifetime_test.h',
            '../../plugins/npapi/test/plugin_npobject_proxy_test.cc',
            '../../plugins/npapi/test/plugin_npobject_proxy_test.h',
            '../../plugins/npapi/test/plugin_schedule_timer_test.cc',
            '../../plugins/npapi/test/plugin_schedule_timer_test.h',
            '../../plugins/npapi/test/plugin_setup_test.cc',
            '../../plugins/npapi/test/plugin_setup_test.h',
            '../../plugins/npapi/test/plugin_thread_async_call_test.cc',
            '../../plugins/npapi/test/plugin_thread_async_call_test.h',
            '../../plugins/npapi/test/plugin_windowed_test.cc',
            '../../plugins/npapi/test/plugin_windowed_test.h',
            '../../plugins/npapi/test/plugin_private_test.cc',
            '../../plugins/npapi/test/plugin_private_test.h',
            '../../plugins/npapi/test/plugin_test_factory.cc',
            '../../plugins/npapi/test/plugin_window_size_test.cc',
            '../../plugins/npapi/test/plugin_window_size_test.h',
            '../../plugins/npapi/test/plugin_windowless_test.cc',
            '../../plugins/npapi/test/plugin_windowless_test.h',
            '../../plugins/npapi/test/resource.h',
          ],
          'include_dirs': [
            '../../..',
          ],
          'xcode_settings': {
            'INFOPLIST_FILE': '<(DEPTH)/webkit/plugins/npapi/test/Info.plist',
          },
          'conditions': [
            ['OS!="win"', {
              'sources!': [
                # TODO(port):  Port these.
                 # plugin_npobject_lifetime_test.cc has win32-isms
                #   (HWND, CALLBACK).
                '../../plugins/npapi/test/plugin_npobject_lifetime_test.cc',
                 # The window APIs are necessarily platform-specific.
                '../../plugins/npapi/test/plugin_window_size_test.cc',
                '../../plugins/npapi/test/plugin_windowed_test.cc',
                 # Seems windows specific.
                '../../plugins/npapi/test/plugin_create_instance_in_paint.cc',
                '../../plugins/npapi/test/plugin_create_instance_in_paint.h',
                 # windows-specific resources
                '../../plugins/npapi/test/npapi_test.def',
                '../../plugins/npapi/test/npapi_test.rc',
              ],
            }],
            ['OS=="mac"', {
              'product_extension': 'plugin',
              'link_settings': {
                'libraries': [
                  '$(SDKROOT)/System/Library/Frameworks/Carbon.framework',
                ],
              },
            }],
            ['os_posix == 1 and OS != "mac" and (target_arch == "x64" or target_arch == "arm")', {
              # Shared libraries need -fPIC on x86-64
              'cflags': ['-fPIC']
            }],
          ],
        },
        {
          'target_name': 'copy_npapi_test_plugin',
          'type': 'none',
          'dependencies': [
            'npapi_test_plugin',
          ],
          'conditions': [
            ['OS=="win"', {
              'copies': [
                {
                  'destination': '<(PRODUCT_DIR)/plugins',
                  'files': ['<(PRODUCT_DIR)/npapi_test_plugin.dll'],
                },
              ],
            }],
            ['OS=="mac"', {
              'copies': [
                {
                  'destination': '<(PRODUCT_DIR)/plugins/',
                  'files': ['<(PRODUCT_DIR)/npapi_test_plugin.plugin'],
                },
              ]
            }],
            ['os_posix == 1 and OS != "mac"', {
              'copies': [
                {
                  'destination': '<(PRODUCT_DIR)/plugins',
                  'files': ['<(PRODUCT_DIR)/libnpapi_test_plugin.so'],
                },
              ],
            }],
          ],
        },
      ],
    }],
  ],
}
