gtest_dep = dependency('gtest', main : true)
test_dummy_exe = executable(
                            'dummy',
                            'test_dummy.cc',
                            include_directories: include_dirs,
                            dependencies: [
                              gtest_dep,
                              deps,
                            ]
                )
test_BlockSpecial_exe = executable(
                                   'BlockSpecial',
                                   '../src/BlockSpecial.cc', 'test_BlockSpecial.cc',
                                   include_directories: include_dirs,
                                   dependencies: [
                                     deps,
                                     gtest_dep,
                                    ]
                        )
test_PipeCapture_exe = executable(
                                  'PipeCapture',
                                  '../src/PipeCapture.cc', 'test_PipeCapture.cc',
                                  include_directories: include_dirs,
                                  dependencies: [
                                    deps,
                                    gtest_dep,
                                ]
                       )
test('test_dummy', test_dummy_exe)
test('test_BlockSpecial', test_BlockSpecial_exe)
test('test_PipeCapture', test_PipeCapture_exe)
