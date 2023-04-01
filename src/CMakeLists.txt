cmake_minimum_required(VERSION 3.14)

file(GLOB_RECURSE SHADERCOMPILER_SRC
    "*.h"
    "*.cpp"
)
list(REMOVE_ITEM SHADERCOMPILER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
add_library(shadercompiler_options INTERFACE)

if(MSVC)
target_compile_options(shadercompiler_options INTERFACE /W4)
target_compile_definitions(shadercompiler_options INTERFACE DEBUG_MODE=$<CONFIG:Debug>)
else()
target_compile_options(shadercompiler_options INTERFACE -Wall -Wextra -Wpedantic -Werror)
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
target_compile_options(shadercompiler_options INTERFACE -g)
target_compile_definitions(shadercompiler_options INTERFACE DEBUG_MODE=1)
else()
        target_compile_options(shadercompiler_options INTERFACE -O2)
        target_compile_definitions(shadercompiler_options INTERFACE DEBUG_MODE=0)
    endif()
    endif()
    
add_library(shadercompiler_lib STATIC ${SHADERCOMPILER_SRC})
add_executable(shadercompiler_run "main.cpp")
target_include_directories(shadercompiler_lib PUBLIC .)
target_link_libraries(shadercompiler_lib shadercompiler_options ${all_libraries})
target_link_libraries(shadercompiler_run shadercompiler_lib shadercompiler_options)

set_target_properties(shadercompiler_run PROPERTIES OUTPUT_NAME shadercompiler)
set_target_properties(shadercompiler_lib PROPERTIES OUTPUT_NAME shadercompiler)

add_custom_command(TARGET shadercompiler_run POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:shadercompiler_run> ${CMAKE_SOURCE_DIR}/bin/)
