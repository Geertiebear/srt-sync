project('srt-sync', 'cpp')

cmake = import('cmake')

incl = include_directories('include')
sources = ['src/main.cpp',
	  'src/Thread.cpp',
	  'src/Client.cpp',
	  'src/Decode.cpp']

libsrt = cmake.subproject('libsrt')
libsrt_shared = libsrt.dependency('srt_shared')

libavcodec = dependency('libavcodec', version: '>=4.0.0')
libavformat = dependency('libavformat', version: '>=4.0.0')
libavutil = dependency('libavutil', version: '>=4.0.0')

executable('srt-sync', sources, include_directories: incl, dependencies: [libsrt_shared, libavformat, libavcodec,
  libavutil], cpp_args: ['-std=c++17'])
