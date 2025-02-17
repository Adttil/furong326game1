cmake_minimum_required (VERSION 3.8)

project(pop_up_book_style_renderer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(MAGIC_CPP_INCLUDE_TESTS OFF)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
if(MSVC)    
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:preprocessor /permissive- /w-")
endif()

include_directories(
    "/" 
    "D:/Mine/Code/RuZhouXieS/zuzhouxie-sequence/senluo/include"
    "D:/Program Files/SDK/VulkanSDK/1.3.224.1/Include"
)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_EXE_LINKER_FLAGS "-static")
endif()

file(GLOB_RECURSE benchmark_srcs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "test/*.cpp")
foreach(srcfile IN LISTS benchmark_srcs)
    get_filename_component(elfname ${srcfile} NAME_WE)
    add_executable(${elfname} ${srcfile})
    target_link_libraries(${elfname}
    "D:/Program Files/SDK/VulkanSDK/1.3.224.1/lib/vulkan-1.lib"
    "D:/Program Files/SDK/glfw-3.3.8.bin.WIN64/lib-vc2022/glfw3.lib")
endforeach()