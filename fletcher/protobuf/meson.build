proto_includes = include_directories('.')

prot_sources = ['expath.pb.cc']
grpc_sources = ['expath.grpc.pb.cc']

prot_libs = static_library('epprot', prot_sources)
grpc_libs = static_library('epgrpc', grpc_sources)


prot_deps = declare_dependency(
    link_with: [prot_libs, grpc_libs],
    include_directories: proto_includes)
