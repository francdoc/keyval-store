cmake_minimum_required(VERSION 3.6) # 3.6 required for regex filtering.

project(multithread C)

# Output the executables and objects to the current directory.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})

# Add the "common" headers as project wide include directory.
include_directories(PUBLIC ../../src)
function(add_sources path)
endfunction()

file(GLOB_RECURSE sources "*.c")
file(GLOB_RECURSE keyval-store_sources "../../src/*.c")
list(APPEND sources ${keyval-store_sources})

# Filter out any unwanted source files that are not part of your project
list(FILTER sources EXCLUDE REGEX "CMakeFiles")

add_executable(multithread.bin ${sources})
target_link_libraries(multithread.bin m pthread)