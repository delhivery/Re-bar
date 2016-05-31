install_headers('marge.hpp')
install_headers('optimal.hpp')
install_headers('pareto.hpp')

margeinc = include_directories('.')
marge_sources = ['graph.cxx', 'optimal.cxx', 'pareto.cxx']
margelib = shared_library(
    'marge', marge_sources,
    dependencies: [ext_dep, bgl_dep, btl_linkdep],
    install: true)
marge_dep = declare_dependency(link_with: margelib, include_directories: margeinc)


pkg_mod = import('pkgconfig')
pkg_mod.generate(
    libraries: margelib, version: '2.0.0',
    name: 'libmarge', filebase: 'marge',
    description: 'The solver libraries behind EP')
