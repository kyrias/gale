project('gale', 'c', version : '1.1happy')

prefix = get_option('prefix')
sysconfdir = join_paths([prefix, get_option('sysconfdir')])
bindir = join_paths([prefix, get_option('bindir')])
datadir = join_paths([prefix, get_option('datadir')])

conf_data = configuration_data()
c = meson.get_compiler('c')

conf_data.set_quoted('VERSION', meson.project_version())

conf_data.set('SIZEOF_INT', c.sizeof('int'))
conf_data.set('SIZEOF_SHORT', c.sizeof('short'))
conf_data.set('SIZEOF_LONG', c.sizeof('long'))

adns = c.find_library('adns', required: false)
conf_data.set('HAVE_ADNS', adns.found())

iconv = c.find_library('iconv', required: false)
conf_data.set('HAVE_ICONV', iconv.found())
if c.has_function('iconv')
  conf_data.set('HAVE_ICONV', true)
endif

readline = c.find_library('readline', required: false)
conf_data.set('HAVE_LIBREADLINE', readline.found())
conf_data.set('HAVE_READLINE_READLINE_H', c.has_header('readline/readline.h'))

ncursesw = dependency('ncursesw', required: false)
conf_data.set('HAVE_LIBNCURSES', ncursesw.found())

subdir('include')
subdir('liboop')
subdir('libgale')
subdir('server')
subdir('gdomain')
subdir('gsend')
subdir('gsub')
subdir('kutils')

install_data('COPYING', install_dir: join_paths([datadir, 'gale']))

script_conf = configuration_data()
script_conf.set('sysconfdir', sysconfdir)
script_conf.set('bindir', bindir)
script_conf.set('VERSION', meson.project_version())
gale_install = configure_file(input: 'gale-install.in', output: 'gale-install', configuration: script_conf)
install_data([gale_install],
             install_dir: bindir,
             install_mode: 'rwxr-xr-x')

# Create the /etc/gale/auth directory tree
meson.add_install_script('create-gale-auth-tree', join_paths([sysconfdir, 'gale']))
