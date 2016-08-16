struct_includes = include_directories('.')
struct_sources = ['structures.cxx']
struct_library = static_library('epstructs', struct_sources)

struct_deps = declare_dependency(
    link_with: struct_library,
    include_directories: struct_includes)
