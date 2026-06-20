cmake_minimum_required(VERSION 3.10)
project(OwnedC C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Treat warnings as errors for safety
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic")
endif()

# Include directories
include_directories(include)

find_package(Threads REQUIRED)

# Core library
add_library(ownedc SHARED
    src/runtime.c
    src/diagnostics.c
    src/collections.c
    src/region.c
    src/ownedc_string.c
    src/ownedc_array.c
    src/ownedc_rc.c
    src/ownedc_file.c
    src/ownedc_mutex.c
    src/ownedc_socket.c
    src/ownedc_thread.c
    src/ownedc_future.c
    src/ownedc_channel.c
    src/ownedc_arena.c
)
target_link_libraries(ownedc Threads::Threads)

# Bare-Metal Kernel Mode Target (NO_STDLIB)
add_library(ownedc_kernel STATIC
    src/runtime.c
    src/diagnostics.c
    src/collections.c
    src/region.c
    src/ownedc_string.c
    src/ownedc_array.c
    src/ownedc_rc.c
    src/ownedc_future.c
)
target_compile_definitions(ownedc_kernel PUBLIC OWNEDC_NO_STDLIB)
target_include_directories(ownedc_kernel PUBLIC include)

# Examples
add_executable(demo examples/demo.c)
target_link_libraries(demo ownedc Threads::Threads)

add_executable(demo_hybrid_cheri examples/demo_hybrid_cheri.c)
target_link_libraries(demo_hybrid_cheri ownedc Threads::Threads)

add_executable(demo_full_showcase examples/demo_full_showcase.c)
target_link_libraries(demo_full_showcase ownedc Threads::Threads)

add_executable(demo_raii examples/demo_raii.c)
target_link_libraries(demo_raii ownedc Threads::Threads)

add_executable(demo_vector examples/demo_vector.c)
target_link_libraries(demo_vector ownedc Threads::Threads)

add_executable(demo_region examples/demo_region.c)
target_link_libraries(demo_region ownedc Threads::Threads)

add_executable(demo_threads examples/demo_threads.c)
target_link_libraries(demo_threads ownedc Threads::Threads)

add_executable(owned_http_server examples/owned_http_server.c)
target_link_libraries(owned_http_server ownedc Threads::Threads)

add_executable(demo_string examples/demo_string.c)
target_link_libraries(demo_string ownedc)

add_executable(demo_array examples/demo_array.c)
target_link_libraries(demo_array ownedc)

add_executable(demo_rc examples/demo_rc.c)
target_link_libraries(demo_rc ownedc)

add_executable(demo_file examples/demo_file.c)
target_link_libraries(demo_file ownedc)

add_executable(demo_mutex examples/demo_mutex.c)
target_link_libraries(demo_mutex ownedc Threads::Threads)

add_executable(demo_allocator examples/demo_allocator.c)
target_link_libraries(demo_allocator ownedc)

add_executable(demo_socket examples/demo_socket.c)
target_link_libraries(demo_socket ownedc)

add_executable(demo_result examples/demo_result.c)
target_link_libraries(demo_result ownedc)

add_executable(demo_thread_safe examples/demo_thread_safe.c)
target_link_libraries(demo_thread_safe ownedc Threads::Threads)

add_executable(demo_rc_cycle examples/demo_rc_cycle.c)
target_link_libraries(demo_rc_cycle ownedc)

add_executable(demo_kernel examples/demo_kernel.c)
target_link_libraries(demo_kernel ownedc_kernel)

add_executable(demo_generics examples/demo_generics.c)
target_link_libraries(demo_generics ownedc)

add_executable(demo_future examples/demo_future.c)
target_link_libraries(demo_future ownedc)

add_executable(demo_profiler examples/demo_profiler.c)
target_link_libraries(demo_profiler ownedc)

add_executable(demo_channel examples/demo_channel.c)
target_link_libraries(demo_channel ownedc)

add_executable(demo_cpp examples/demo_cpp.cpp)
target_link_libraries(demo_cpp ownedc)

add_executable(demo_arena examples/demo_arena.c)
target_link_libraries(demo_arena ownedc)

# Testing
enable_testing()

# Normal tests
add_executable(benchmark tests/benchmark.c)
target_link_libraries(benchmark ownedc Threads::Threads)

add_executable(test_hybrid_compile tests/test_hybrid_compile.c)
target_link_libraries(test_hybrid_compile ownedc Threads::Threads)
add_test(NAME HybridCompileTest COMMAND test_hybrid_compile)

# Normal tests
add_executable(test_malloc tests/test_malloc.c)
target_link_libraries(test_malloc ownedc Threads::Threads)
add_test(NAME MallocTest COMMAND test_malloc)

add_executable(test_raii tests/test_raii.c)
target_link_libraries(test_raii ownedc Threads::Threads)
add_test(NAME RAIITest COMMAND test_raii)

add_executable(test_region tests/test_region.c)
target_link_libraries(test_region ownedc Threads::Threads)
add_test(NAME RegionTest COMMAND test_region)

add_executable(test_threads tests/test_threads.c)
target_link_libraries(test_threads ownedc Threads::Threads)
add_test(NAME ThreadsTest COMMAND test_threads)

# Tests that intentionally fail
add_executable(test_leaks tests/test_leaks.c)
target_link_libraries(test_leaks ownedc Threads::Threads)
add_test(NAME LeaksTest COMMAND test_leaks)
set_tests_properties(LeaksTest PROPERTIES WILL_FAIL TRUE)

add_executable(test_uaf tests/test_uaf.c)
target_link_libraries(test_uaf ownedc Threads::Threads)
add_test(NAME UAFTest COMMAND test_uaf)
set_tests_properties(UAFTest PROPERTIES WILL_FAIL TRUE)

add_executable(test_doublefree tests/test_doublefree.c)
target_link_libraries(test_doublefree ownedc Threads::Threads)
add_test(NAME DoubleFreeTest COMMAND test_doublefree)
set_tests_properties(DoubleFreeTest PROPERTIES WILL_FAIL TRUE)

add_executable(test_borrow tests/test_borrow.c)
target_link_libraries(test_borrow ownedc Threads::Threads)
add_test(NAME BorrowTest COMMAND test_borrow)
set_tests_properties(BorrowTest PROPERTIES WILL_FAIL TRUE)

add_executable(test_vector tests/test_vector.c)
target_link_libraries(test_vector ownedc Threads::Threads)
add_test(NAME VectorTest COMMAND test_vector)
set_tests_properties(VectorTest PROPERTIES WILL_FAIL TRUE)

add_executable(test_thread_violation tests/test_thread_violation.c)
target_link_libraries(test_thread_violation ownedc Threads::Threads)
add_test(NAME ThreadViolationTest COMMAND test_thread_violation)
set_tests_properties(ThreadViolationTest PROPERTIES WILL_FAIL TRUE)

