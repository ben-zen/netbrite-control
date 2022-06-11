project('netbrite-control', 'cpp',
  version : '0.1',
  default_options : ['cpp_std=c++20'])

lib = library('nbx', 'nb_connect.cc')

executable('nbctl', 'main.cc', link_with: lib)