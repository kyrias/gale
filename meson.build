project('gale', 'c', version : '1.1happy')

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

readline = c.find_library('readline', required: false)
conf_data.set('HAVE_LIBREADLINE', readline.found())
conf_data.set('HAVE_READLINE_READLINE_H', c.has_header('readline/readline.h'))

ncursesw = dependency('ncursesw', required: false)
conf_data.set('HAVE_LIBNCURSES', ncursesw.found())

subdir('include')
subdir('liboop')
subdir('libgale')
