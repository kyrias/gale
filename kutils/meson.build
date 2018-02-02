gkinfo_sources = ['gkinfo.c']
gkinfo = executable('gkinfo',
                    gkinfo_sources,
                    include_directories: includes,
                    dependencies: [liboop, libgale])

gkgen_sources = ['gkgen.c']
gkgen = executable('gkgen',
                   gkgen_sources,
                   include_directories: includes,
                   dependencies: [liboop, libgale])

gksign_sources = ['gksign.c']
gksign = executable('gksign',
                    gksign_sources,
                    include_directories: includes,
                    dependencies: [liboop, libgale])

install_data(['ROOT'],
             install_dir: join_paths(sysconfdir, 'gale', 'auth', 'trusted'))
