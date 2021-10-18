cmake_minimum_required(VERSION 3.10.2)
project(qctools)

if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
   message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
   file(
        DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.15/conan.cmake"
        "${CMAKE_BINARY_DIR}/conan.cmake"
        TLS_VERIFY ON
    )
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

conan_cmake_run(
    BASIC_SETUP
    CMAKE_TARGETS
    REQUIRES
        catch2/2.13.1
)
find_package(Threads REQUIRED)

add_executable(nc no_copy/main.cpp)
add_executable(zip zip/main.cpp)

add_executable(tsafe tsafe/tests.cpp)
target_link_libraries(tsafe CONAN_PKG::catch2 Threads::Threads)
