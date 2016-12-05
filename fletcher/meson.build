project('fletcher', 'cpp', version: '2.5.0', license: 'Delhivery', default_options:['cpp_std=c++1z'])

subdir('structures')


PROJECT_EXECUTABLE_NAME='fletcher'
fletcher_exe = executable(
    PROJECT_EXECUTABLE_NAME, 'main.cxx',
    dependencies: [struct_deps],
    link_with: [struct_library],
    install: true)
