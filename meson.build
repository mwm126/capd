project('capd', 'c',
  version : '1.04-dev',
  default_options : ['warning_level=3'])


cc = meson.get_compiler('c')
cryptolib = cc.find_library('crypto')

executable('capd',
           'capd.c',
           dependencies: cryptolib,
           install : true)
