project(
  'capd', 'c', 'cpp',
  version : files('VERSION'),
  default_options : ['werror=true', 'c_std=c2x', 'warning_level=3'])

cc = meson.get_compiler('c')
cryptolib = cc.find_library('crypto')


subdir('src')

executable(
  'capd',
  main_src,
  dependencies: cryptolib,
  link_with: capd_lib,
  install : true
)

capd_toml = '/home/mark/repos/capd/etc/capd.toml'
subdir('test')

install_data('etc/capd.counter', install_dir: '/etc/capd')
install_data('etc/capd.passwd', install_dir: '/etc/capd')
install_data(capd_toml, install_dir: '/etc/capd')
install_data('openClose.sh', install_dir: 'bin')
