cmake_minimum_required(VERSION 3.16)
project(ds_lib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The library itself is header-only (templates), so it's an INTERFACE target:
# it has no .cpp files to compile, it just tells users where the headers are.
add_library(ds INTERFACE)
target_include_directories(ds INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(ds_tests
    tests/test_main.cpp
    tests/test_vector.cpp
    tests/test_linkedlist.cpp
    tests/test_hashmap.cpp
)
target_link_libraries(ds_tests PRIVATE ds)

add_executable(ds_demo examples/demo.cpp)
target_link_libraries(ds_demo PRIVATE ds)

# Strict warnings on everything we compile.
foreach(target ds_tests ds_demo)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
    endif()
endforeach()

# Optional: catch leaks and bad memory access at runtime (GCC/Clang on Linux/macOS).
#   cmake -B build -DDS_SANITIZE=ON
option(DS_SANITIZE "Build tests with AddressSanitizer" OFF)
if(DS_SANITIZE AND NOT MSVC)
    target_compile_options(ds_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(ds_tests PRIVATE -fsanitize=address,undefined)
endif()

enable_testing()
add_test(NAME ds_tests COMMAND ds_tests)
