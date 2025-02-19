cmake_minimum_required (VERSION 3.8)

project(furong326game1)

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
    "3rd_party/"    
)

link_directories(
    "/"
    "3rd_party/"
)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_EXE_LINKER_FLAGS "-static")
endif()

file(GLOB_RECURSE benchmark_srcs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "test/*.cpp")
foreach(srcfile IN LISTS benchmark_srcs)
    get_filename_component(elfname ${srcfile} NAME_WE)
    file(GLOB test_srcs ${srcfile} 
        "3rd_party/imgui/*.cpp"
        "3rd_party/stb_image/stb_image.cpp"
    )
    add_executable(${elfname} ${test_srcs})
    target_link_libraries(${elfname}
        "vulkan/vulkan-1"
        "GLFW/glfw3"
    )
endforeach()