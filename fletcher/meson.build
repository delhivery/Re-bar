project('fletcher', 'cpp', version: '2.5.0', license: 'Delhivery', default_options:['cpp_std=c++1z'])

subdir('protobuf')
subdir('structures')


proto_linkdep = dependency('protobuf')
grpc_linkdep = dependency('grpc++_unsecure')

PROJECT_EXECUTABLE_NAME='fletcher'
fletcher_exe = executable(
    PROJECT_EXECUTABLE_NAME, 'main.cxx',
    dependencies: [prot_deps, struct_deps, grpc_linkdep, proto_linkdep],
    link_with: [struct_library, prot_libs, grpc_libs],
    install: true)
