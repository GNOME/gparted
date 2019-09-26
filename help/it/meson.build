mo = custom_target(
  l + '_lingua_mo',
  input: l + '.po',
  output: l + '.mo',
  command: msgfmt_command,
  install: false
)

xml = custom_target(
  l + '_lingua_xml',
  input: [mo, '../C/gparted.xml'],
  output: 'gparted.xml',
  command: xml2po_command,
  install: true,
  install_dir: join_paths(gnome_help_prefix, l)
)

custom_target(
  l + '_lingua_omf',
  input: xml,
  output: 'gparted-' + l + '.omf',
  command: xsltproc_command,
  install: true,
  install_dir: join_paths(get_option('prefix'),get_option('datadir'), 'omf', 'gparted')
)
if run_command('[', '-d', 'figures', ']').returncode() == 0
  install_data(
    'figures/gparted_window.png',
    install_dir: join_paths(gnome_help_prefix, l, 'figures')
  )
endif