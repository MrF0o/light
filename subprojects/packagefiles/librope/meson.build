project('librope', 'c',
  version: '0.1.0',
  license: 'MIT',
  default_options: [
    'c_std=c11',
    'warning_level=0',
    'werror=false'
  ]
)

librope_sources = files(
  'rope.c',
)

librope_inc = include_directories('.')

librope_lib = static_library('rope',
  librope_sources,
  include_directories: librope_inc,
  install: false
)

librope_dep = declare_dependency(
  link_with: librope_lib,
  include_directories: librope_inc
)
