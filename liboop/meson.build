c = meson.get_compiler('c')
adns = c.find_library('adns')


liboop_includes = include_directories('.')


liboop_sources = ['sys.c', 'select.c', 'signal.c', 'alloc.c',
                  'read.c', 'read-fd.c', 'read-mem.c']
liboop_lib = shared_library('oop',
                            liboop_sources,
                            version: '4.0.1')
liboop = declare_dependency(link_with: liboop_lib,
                            include_directories: liboop_includes)


liboop_adns_sources = ['adns.c']
liboop_adns_lib = shared_library('oop-adns',
                                 liboop_adns_sources,
                                 version: '3.0.0',
                                 c_args: '-DHAVE_ADNS',
                                 dependencies: [liboop, adns])
liboop_adns = declare_dependency(link_with: liboop_adns_lib,
                                 include_directories: liboop_includes)
