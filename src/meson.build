incdir = include_directories('.')
capd_lib = static_library(
  'authProcess.c',
  'coreProcess.c',
  'counterFunctions.c',
  'cryptoFunctions.c',
  'globalDefinitions.c',
  'netProcess.c',
  'replayFunctions.c',
  'utilityFunctions.c',
  gnu_symbol_visibility:'default',
)

main_src = files('capd.c')
