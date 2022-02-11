unity = subproject('unity')
unity_dep = unity.get_variable('unity_dep')

test_exe = executable(
  'the_test', 'test.c',
  dependencies : unity_dep)

test('name of test', test_exe)
