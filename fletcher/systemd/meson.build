conf_data = configuration_data()
conf_data.set('BIN_INSTALL_DIR', get_option('prefix') + '/' + get_option('bindir'))
conf_data.set('BIN_FILE_NAME', FLETCHER_EXECUTABLE_NAME)

configure_file(input: 'fletcher.service.in', output: 'fletcher.service', configuration: conf_data, install_dir: '/usr/lib/systemd/system')
