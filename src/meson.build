toml_dep = dependency('tomlplusplus')

incdir = include_directories('.')
capd_lib = static_library(
  'capd',
  'authProcess.c',
  'coreProcess.c',
  'counterFunctions.c',
  'cryptoFunctions.c',
  'globalDefinitions.c',
  'netProcess.c',
  'parse_config.cpp',
  'replayFunctions.c',
  'utilityFunctions.c',
  dependencies: [cryptolib, toml_dep],
  c_args: [
    '-D_DEFAULT_SOURCE',
    '-DCAPD_VERSION='+meson.project_version()
  ],
)


main_src = files('capd.c')
