cmake_minimum_required(VERSION 3.7)

project(wavetables)

find_package(PkgConfig REQUIRED)
find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_subdirectory(external/kiss_fft EXCLUDE_FROM_ALL)

pkg_check_modules(Sndfile "sndfile" REQUIRED IMPORTED_TARGET)

add_executable(wavetables src/main.cpp src/waveview.h src/waveview.cpp)
target_include_directories(wavetables PRIVATE src)
target_link_libraries(wavetables PRIVATE Qt5::Widgets sfizz-kissfft PkgConfig::Sndfile)
set_target_properties(wavetables PROPERTIES AUTOUIC ON)
