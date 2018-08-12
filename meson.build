project(
  'gparted',
  'cpp',
  version : '0.31.0',
  meson_version: '>= 0.46.0',
)

cpp = meson.get_compiler('cpp')

add_project_arguments(
  '-DHAVE_CONFIG_H',
  '-DGNOME_ICONDIR="' + join_paths(get_option('prefix'), get_option('datadir'), 'pixmaps') + '"',
  '-DGNOMELOCALEDIR="' + join_paths(get_option('prefix'), get_option('datadir'), 'locale') + '"',
  language: 'cpp',
)

glibmm_dep = dependency('glibmm-2.4', version: '>=2.14.0')
gtkmm_dep = dependency('gtkmm-2.4', version: '>2.8')
gthread_dep = dependency('gthread-2.0')
libdl_dep = cpp.find_library('dl')
libparted_dep = dependency('libparted', version: '>=1.7.1')
uuid_dep = dependency('uuid')

conf = configuration_data()

gparted_version = meson.project_version()
conf.set_quoted('VERSION', gparted_version)
conf.set_quoted('PACKAGE_VERSION', gparted_version)
conf.set_quoted('PACKAGE_STRING', 'gparted' + gparted_version)

# Check for libparted 2.0 to 3.0 inclusive for a bug where loop table
# creation doesn't delete old partitions.
if dependency('libparted', version: ['>=2', '<=3'], required: false).found()
  conf.set('ENABLE_LOOP_DELETE_OLD_PTNS_WORKAROUND', true)
else
  conf.set('ENABLE_LOOP_DELETE_OLD_PTNS_WORKAROUND', false)
endif

# Check for libparted >= 2.2 for improved informing the kernel to
# re-read the partition table code and support of larger sector sizes
# (> 512 bytes).
if dependency('libparted', version: '>=2.2', required: false).found()
  conf.set('USE_LIBPARTED_LARGE_SECTOR_SUPPORT', 1)
else
  conf.set('ENABLE_PT_REREAD_WORKAROUND', 1)
endif

# Check for ped_file_system_resize() function to determine the existence
# of the API in the original parted library.  Available in parted <= 2.4.
parted_fs_resize_dep = [ ]
if cpp.has_function('ped_file_system_resize', dependencies: libparted_dep)
  conf.set('HAVE_LIBPARTED_FS_RESIZE', 1)
# If not already found, check for ped_file_system_resize() function in
# the parted-fs-resize library to determine the need to use the new
# library.  Available in parted >= 3.1.
elif cpp.find_library('parted-fs-resize', required: false).found()
  parted_fs_resize_dep = cpp.find_library('parted-fs-resize')
  conf.set('HAVE_LIBPARTED_FS_RESIZE', 1)
else
  conf.set('HAVE_LIBPARTED_FS_RESIZE', false)
  parted_fs_resize_dep = [ ]
endif

# Check for gtkmm >= 2.22 to determine availability of Gtk::MessageDialog::get_message_area().
if dependency('gtkmm-2.4', version: '>=2.22.0', required: false).found()
  conf.set('HAVE_GET_MESSAGE_AREA', 1)
endif

# Only enable C++11 compilation if required
cxx_std = 'c++98'

# Check for glibmm >= 2.45.40 which requires C++11 compilation
if dependency('glibmm-2.4', version: '>=2.45.40', required: false).found()
  cxx_std = 'c++11'
endif

# Check for libsigc++ >= 2.5.1 which requires C++11 compilation.
if dependency('sigc++-2.0', version: '>-2.5.1', required: false).found()
  cxx_std = 'c++11'
endif

deps = [
  libdl_dep,
  uuid_dep,
  libparted_dep,
  gthread_dep,
  gtkmm_dep,
  glibmm_dep,
  parted_fs_resize_dep,
]


conf.set('PACKAGE', 'gparted')
conf.set('PACKAGE_NAME', 'gparted')

#TODO: Optionalize the below if necessary
conf.set('ENABLE_NLS', true)
conf.set('HAVE_INTTYPES_H', 1)
conf.set('HAVE_LIBPARTED', 1)
conf.set('HAVE_LIBUUID', 1)
conf.set('HAVE_STRING_H', 1)

if get_option('help')
  conf.set('ENABLE_HELP_DOC', 1)
  subdir('help')
else
  conf.set('ENABLE_HELP_DOC', false)
endif

configure_file(
  output: 'config.h',
  configuration: conf
)

include_dirs = [
  include_directories('include'),
  include_directories('src'),
  include_directories('.'),
]

# i18n stuff
i18n = import('i18n')

intltool = find_program('intltool-merge')

podir = join_paths(meson.current_source_dir(), 'po')

intltool_cache = join_paths(meson.current_build_dir(), 'intltool-merge-cache')

intltool_xml_command = [intltool, '-x', '-u',
                    '-c', intltool_cache,
                    podir, '@INPUT@', '@OUTPUT@']

intltool_desktop_command = [intltool, '-d', '-u',
                    '-c', intltool_cache,
                    podir, '@INPUT@', '@OUTPUT@']

gettext_package = 'gparted'
add_project_arguments('-DGETTEXT_PACKAGE="' + gettext_package + '"', language: 'cpp')
subdir('po')

po_conf = configuration_data()

po_conf.set('bindir', join_paths(get_option('prefix'), get_option('bindir')))

desktop_in = configure_file(
  input: 'gparted.desktop.in.in',
  output: 'gparted.desktop.in',
  configuration: po_conf
)

# TODO: maybe drop intltool and use pure gettext so that we can use i18n.merge_file instead
custom_target(
  'gparted.appdata.xml',
  input: 'gparted.appdata.xml.in',
  output: 'gparted.appdata.xml',
  command: intltool_xml_command,
  install: true,
  install_dir: join_paths(get_option('datadir'), 'appdata')
)

custom_target(
  'gparted.desktop',
  input: desktop_in,
  output: 'gparted.desktop',
  command: intltool_desktop_command,
  install: true,
  install_dir: join_paths(get_option('datadir'), 'applications')
)

if get_option('polkit')
  polkitpolicydir = join_paths(get_option('datadir'), 'polkit-1/actions')

  polkit_policy_in = configure_file(
    input: 'org.gnome.gparted.policy.in.in',
    output: 'org.gnome.gparted.policy.in',
    configuration: po_conf
  )

  custom_target(
    'org.gnome.gparted.policy',
    input: polkit_policy_in,
    output: 'org.gnome.gparted.policy',
    command: intltool_xml_command,
    install: true,
    install_dir: polkitpolicydir
  )
endif

# Find graphical privilege escalation program
gksuprog = ''
if get_option('polkit')
  # Check for pkexec >= 0.102 for it's ability to run X11 apps.
  polkit_dep = dependency('polkit-agent-1', version: '>=0.102')
  gksuprog = 'pkexec --disable-internal-agent'
elif find_program('gksudo', required: false).found()
  gksuprog = 'gksudo'
elif find_program('gksu', required: false).found()
  gksuprog = 'gksu'
elif find_program('kdesudo', required: false).found()
  gksuprog = 'kdesudo'
elif find_program('xdg-su', required: false).found()
  gksuprog = 'xdg-su -c'
endif

# Configure the gparted script
gparted_bin_conf = configuration_data()

gparted_bin_conf.set('sbindir', join_paths(get_option('prefix'), get_option('bindir')))
gparted_bin_conf.set('gksuprog', gksuprog)
gparted_bin_conf.set('enable_xhost_root', get_option('xhost_root'))
gparted_bin_conf.set('bindir', join_paths(get_option('prefix'), get_option('bindir')))

gparted_bin = configure_file(
  input: 'gparted.in',
  output: 'gparted',
  configuration: gparted_bin_conf
)

install_data(
  gparted_bin,
  install_dir: join_paths(get_option('bindir')),
  rename: 'gparted',
  install_mode: 'rwxr-xr-x'
)

# Optional stuff
if get_option('libparted_dmraid')
  conf.set('USE_LIBPARTED_DMRAID', 1)
endif

# Check for libparted >= 3.2 for online resize support
if get_option('online-resize')
  dependency('libparted', version: '>=3.2')
  conf.set('ENABLE_ONLINE_RESIZE', 1)
endif

if get_option('man')
  subdir('doc')
endif

subdir('data')
subdir('src')
subdir('tests')