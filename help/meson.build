linguas = [
  'C',
  'cs',
  'de',
  'el',
  'es',
  'fr',
  'gd',
  'it',
  'pt_BR',
  'ro',
  'ru',
  'sl',
  'sr',
  'sr@latin',
  'sv',
  'te',
  'th',
]

# We use these later in the individual meson.build's of every lingua.
# Meson doesn't support outputting files into custom directories with
# custom_target via the "output" argument, so we need one meson.build for
# every lingua, otherwise we'd have to call each file differently.
msgfmt = find_program('msgfmt')
msgfmt_command = [msgfmt, '-o', '@OUTPUT@', '@INPUT@']
xml2po = find_program('xml2po')
xml2po_command = [xml2po, '-o', '@OUTPUT@', '-m', 'docbook', '-e', '-t', '@INPUT0@', '@INPUT1@']
xsltproc = find_program('xsltproc')
scrollkeeper_config = find_program('scrollkeeper-config')
pkgconfig = find_program('pkg-config')
scrollkeeper_output = run_command(scrollkeeper_config, '--pkgdatadir').stdout().strip()
pkgconfig_output = run_command(pkgconfig, '--variable', 'db2omf', 'gnome-doc-utils').stdout().strip()
gnome_help_prefix = join_paths(get_option('prefix'), get_option('datadir'), 'gnome', 'help', 'gparted')

foreach l: linguas
  # Define it here, so that we can use the "l" variable for the lingua
  xsltproc_command = [xsltproc, '-o', '@OUTPUT@', '--stringparam', 'db2omf.basename', 'gparted', '--stringparam', 'db2omf.format', 'docbook', '--stringparam', 'db2omf.dtd', '-//OASIS//DTD DocBook XML V4.1.2//EN', '--stringparam', 'db2omf.lang', l, '--stringparam', 'db2omf.omf_dir', join_paths(get_option('prefix'),get_option('datadir'), 'omf'), '--stringparam', 'db2omf.help_dir', join_paths(join_paths(get_option('prefix'),get_option('datadir')), 'gnome/help'), '--stringparam', 'db2omf.omf_in', join_paths(meson.current_source_dir(), 'gparted.omf.in'), '--stringparam', 'db2omf.scrollkeeper_cl', scrollkeeper_output + '/Templates/C/scrollkeeper_cl.xml', pkgconfig_output, '@INPUT@']
  subdir(l)
endforeach