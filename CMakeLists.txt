cmake_minimum_required(VERSION 3.13)

project(c_buffer C ASM)

add_library(c_buffer INTERFACE)

target_sources(c_buffer INTERFACE
	src/c_buffer.c
)

target_include_directories(c_buffer INTERFACE
	src
)

# Option to build standalone executable for testing
option(C_BUFFER_TEST "Build standalone executable for c_buffer" OFF)

if(C_BUFFER_TEST)
    set(CMAKE_C_COMPILER gcc)

    # Add standalone executable for testing c_buffer
    add_executable(test_c_buffer test/test_c_buffer.c)

    # Link the c_buffer library to the standalone executable
    target_link_libraries(test_c_buffer PRIVATE c_buffer)

    # Optionally, add any specific compiler options for testing
    target_compile_options(test_c_buffer PRIVATE -Wall -Wextra -pedantic)
endif()