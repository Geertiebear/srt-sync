project('srt-sync', 'cpp')

cmake = import('cmake')

incl = include_directories('include')
sources = ['src/main.cpp',
	  'src/Thread.cpp',
	  'src/Client.cpp']

libsrt = cmake.subproject('libsrt')
libsrt_shared = libsrt.dependency('srt_shared')

executable('srt-sync', sources, include_directories: incl, dependencies: [libsrt_shared],
  cpp_args: ['-std=c++17'])
