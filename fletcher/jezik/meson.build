jezikinc = include_directories('.')
jezik_dep = declare_dependency(include_directories: jezikinc)

jezik_sources = ['jezik.cxx']
jeziklib = static_library(
    'jezik', jezik_sources,
    dependencies: ext_dep,
    install: false)

