install_data(
  'gparted.xml',
  install_dir: join_paths(gnome_help_prefix, l)
)

custom_target(
  l + '_lingua_omf',
  input: 'gparted.xml',
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