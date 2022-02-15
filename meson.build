project('capd', 'c',
  version : files('VERSION'),
  default_options : ['c_std=c2x', 'warning_level=3'])


cc = meson.get_compiler('c')
cryptolib = cc.find_library('crypto')

incdir = include_directories('.')
proc_srcs = files(
  'authProcess.c',
  'coreProcess.c',
  'counterFunctions.c',
  'cryptoFunctions.c',
  'globalDefinitions.c',
  'netProcess.c',
  'replayFunctions.c',
  'utilityFunctions.c',
)

executable('capd',
           'capd.c',
           proc_srcs,
           dependencies: cryptolib,
           c_args: [
             '-D_DEFAULT_SOURCE',
             '-DCAPD_VERSION='+meson.project_version()
           ],
           install : true)

subdir('test')
