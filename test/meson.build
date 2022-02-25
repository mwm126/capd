unity = subproject('unity')
unity_dep = unity.get_variable('unity_dep')

auth_test_exe = executable(
  'test_auth', 'test_auth.c',
  link_with: capd_lib,
  include_directories: incdir,
  c_args: ['-D_DEFAULT_SOURCE'],
  dependencies : [cryptolib, unity_dep])

glob_test_exe = executable(
  'test_glob', 'test_globalDefinitions.c',
  link_with: capd_lib,
  include_directories: incdir,
  c_args: ['-D_DEFAULT_SOURCE'],
  dependencies : [cryptolib, unity_dep])

test('auth tests', auth_test_exe)
test('global tests', glob_test_exe)
