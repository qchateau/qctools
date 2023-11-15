cmake_minimum_required(VERSION 3.10.2)
project(qctools)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
   message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
   file(
        DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/0.18.1/conan.cmake"
        "${CMAKE_BINARY_DIR}/conan.cmake"
        TLS_VERIFY ON
    )
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)
conan_cmake_run(
    REQUIRES
        catch2/3.4.0
        asio/1.28.2
    BASIC_SETUP CMAKE_TARGETS
    BUILD missing
)

find_package(Threads REQUIRED)

add_executable(nc no_copy/main.cpp)
set_property(TARGET nc PROPERTY CXX_STANDARD 14)

add_executable(zip zip/main.cpp)
set_property(TARGET zip PROPERTY CXX_STANDARD 14)

add_executable(to_struct to_struct/main.cpp)
set_property(TARGET to_struct PROPERTY CXX_STANDARD 14)

add_executable(tsafe tsafe/tests.cpp)
set_property(TARGET tsafe PROPERTY CXX_STANDARD 17)
target_link_libraries(tsafe CONAN_PKG::catch2 Threads::Threads)

add_executable(atools atools/main.cpp)
set_property(TARGET atools PROPERTY CXX_STANDARD 20)
target_link_libraries(atools CONAN_PKG::asio)
