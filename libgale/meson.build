gale_sys_dir = join_paths([get_option('sysconfdir'), 'gale'])

bdw_gc = dependency('bdw-gc')
libcrypto = dependency('libcrypto')

libgale_sources = ['core_init.c', 'core_link.c', 'core_signals.c',
                   'io_input.c', 'io_output.c',
                   'client_alias.c', 'client_code.c', 'client_default.c', 'client_i.c', 'client_location.c',
                   'client_pack.c', 'client_queue.c', 'client_server.c', 'client_standard.c', 'client_unpack.c',
                   'crypto_gen.c', 'crypto_i.c', 'crypto_misc.c', 'crypto_seal.c', 'crypto_sign.c', 'crypto_sign_raw.c',
                   'key_assert.c', 'key_generate.c', 'key_graph.c', 'key_handle.c', 'key_i.c',
                   'key_search.c', 'key_search_akd.c', 'key_search_builtin.c', 'key_search_dirs.c',
                   'misc_alloc.c', 'misc_charset.c', 'misc_connect.c', 'misc_debug.c',
                   'misc_dir.c', 'misc_envvar.c', 'misc_error.c', 'misc_exec.c', 'misc_file.c',
                   'misc_fragment.c', 'misc_globals.c', 'misc_kill.c', 'misc_map.c', 'misc_pack.c',
                   'misc_readline.c', 'misc_report.c', 'misc_terminal.c', 'misc_text.c', 'misc_time.c',
                   'wcwidth.c']
libgale_lib = shared_library('gale',
                             libgale_sources,
                             include_directories: includes,
                             c_args : ['-DGALE_SYS_DIR="@0@"'.format(gale_sys_dir)],
                             dependencies : [readline, liboop, liboop_adns, ncursesw, libcrypto, bdw_gc])
libgale = declare_dependency(link_with: libgale_lib,
                             include_directories: includes)
