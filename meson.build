project(
  'capd', 'c',
  version : files('VERSION'),
  default_options : ['werror=true', 'c_std=c2x', 'warning_level=3'])

cc = meson.get_compiler('c')
cryptolib = cc.find_library('crypto')

subdir('src')
subdir('test')

executable(
  'capd',
  main_src,
  dependencies: cryptolib,
  link_with: capd_lib,
  install : true
)

install_data('openClose.sh', install_dir: 'bin')
