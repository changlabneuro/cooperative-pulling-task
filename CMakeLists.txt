cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 17)

project(om)
add_library(${PROJECT_NAME} STATIC)

link_directories(lib/win/${CMAKE_BUILD_TYPE})
link_libraries(serial)
include_directories(deps/serial/include)

function(configure_compiler_flags project)
    if(MSVC)
        target_compile_options(${project} PRIVATE /W4)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${project} PRIVATE -Wall -Wextra -pedantic)
    else()
        target_compile_options(${project} PRIVATE -Wall -Wextra -pedantic -Werror)
    endif()
endfunction(configure_compiler_flags)

target_include_directories(${PROJECT_NAME} PUBLIC
        src
        deps/stb
        deps/imgui
        deps/glad/include)

target_sources(${PROJECT_NAME} PUBLIC
        ${CMAKE_SOURCE_DIR}/src/common/serial.hpp
        ${CMAKE_SOURCE_DIR}/src/common/serial.cpp
        ${CMAKE_SOURCE_DIR}/src/common/serial_lever.hpp
        ${CMAKE_SOURCE_DIR}/src/common/serial_lever.cpp
        ${CMAKE_SOURCE_DIR}/src/common/lever_system.hpp
        ${CMAKE_SOURCE_DIR}/src/common/lever_system.cpp
        ${CMAKE_SOURCE_DIR}/src/common/glfw.hpp
        ${CMAKE_SOURCE_DIR}/src/common/glfw.cpp
        ${CMAKE_SOURCE_DIR}/src/common/imgui.hpp
        ${CMAKE_SOURCE_DIR}/src/common/imgui.cpp
        ${CMAKE_SOURCE_DIR}/src/common/identifier.hpp
        ${CMAKE_SOURCE_DIR}/src/common/ringbuffer.hpp
        ${CMAKE_SOURCE_DIR}/src/common/handshake.hpp
        ${CMAKE_SOURCE_DIR}/src/common/vector.hpp
        ${CMAKE_SOURCE_DIR}/src/common/render.hpp
        ${CMAKE_SOURCE_DIR}/src/common/render.cpp

        #   GLAD
        ${CMAKE_SOURCE_DIR}/deps/glad/src/glad.c

        #   IMGUI
        ${CMAKE_SOURCE_DIR}/deps/imgui/imgui.cpp
        ${CMAKE_SOURCE_DIR}/deps/imgui/imgui_draw.cpp
        ${CMAKE_SOURCE_DIR}/deps/imgui/imgui_tables.cpp
        ${CMAKE_SOURCE_DIR}/deps/imgui/imgui_widgets.cpp
        ${CMAKE_SOURCE_DIR}/deps/imgui/imgui_demo.cpp
        ${CMAKE_SOURCE_DIR}/deps/imgui/backends/imgui_impl_glfw.cpp
        ${CMAKE_SOURCE_DIR}/deps/imgui/backends/imgui_impl_opengl3.cpp)

configure_compiler_flags(${PROJECT_NAME})

add_subdirectory(src/sandbox)
add_subdirectory(deps/glfw)

target_link_libraries(${PROJECT_NAME} PUBLIC glfw)