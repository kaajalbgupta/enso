if dev_backend == 'intel_fpga'
    library_name = 'enso'
elif dev_backend == 'hybrid'
    library_name = 'enso_hybrid'
else
    error('Unknown backend')
endif

if latency_opt == false
    library_name += '_no_latency_opt'
endif

inc = include_directories('include')

subdir('include')
subdir('src')

enso_lib = library(library_name, project_sources, install: true,
                   include_directories: [inc, backend_inc])
pkg_mod = import('pkgconfig')
pkg_mod.generate(enso_lib)

subdir('examples')
subdir('test')
