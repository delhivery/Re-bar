project('fletcher', 'cpp', version: '2.0.0', license: 'Delhivery', default_options: ['cpp_std=c++14'])

ext_dep = declare_dependency(include_directories: include_directories('external'))
bgl_dep = dependency('boost', modules: ['graph'])
btl_linkdep = dependency('boost', modules: ['thread'])


FLETCHER_EXECUTABLE_NAME = 'fletcher'

subdir('marge')
subdir('jezik')
subdir('systemd')

fletcher_exe = executable(
    FLETCHER_EXECUTABLE_NAME, 'fletcher.cxx',
    dependencies: [ext_dep, marge_dep, jezik_dep, btl_linkdep],
    link_with: [margelib, jeziklib],
    install: true)
test('fletcher test', fletcher_exe)
