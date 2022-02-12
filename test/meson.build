unity = subproject('unity')
unity_dep = unity.get_variable('unity_dep')

test_exe = executable(
  'test_auth', 'test_auth.c', proc_srcs,
  include_directories: incdir,
  c_args: ['-D_DEFAULT_SOURCE'],
  dependencies : [cryptolib, unity_dep])

test('name of test', test_exe)
