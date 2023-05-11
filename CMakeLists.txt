cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 17)

project(om)
add_library(${PROJECT_NAME} STATIC)

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
        deps
        deps/stb
        deps/imgui
        deps/implot
        deps/glad/include
        deps/serial/include
        deps/json/include)

set(NI_DIR "C:\\Program Files (x86)\\National Instruments\\Shared\\ExternalCompilerSupport\\C")

target_include_directories(${PROJECT_NAME} PUBLIC
    ${NI_DIR}/include
)

target_link_directories(${PROJECT_NAME} PUBLIC
    ${NI_DIR}/lib64/msvc
)

target_sources(${PROJECT_NAME} PUBLIC
        ${CMAKE_SOURCE_DIR}/src/common/app.hpp
        ${CMAKE_SOURCE_DIR}/src/common/app.cpp
        ${CMAKE_SOURCE_DIR}/src/common/audio.hpp
        ${CMAKE_SOURCE_DIR}/src/common/audio.cpp
        ${CMAKE_SOURCE_DIR}/src/common/common.hpp
        ${CMAKE_SOURCE_DIR}/src/common/ni.hpp
        ${CMAKE_SOURCE_DIR}/src/common/ni.cpp
        ${CMAKE_SOURCE_DIR}/src/common/led.hpp
        ${CMAKE_SOURCE_DIR}/src/common/led.cpp
        ${CMAKE_SOURCE_DIR}/src/common/ni_gui.hpp
        ${CMAKE_SOURCE_DIR}/src/common/ni_gui.cpp
        ${CMAKE_SOURCE_DIR}/src/common/sample_queue.hpp
        ${CMAKE_SOURCE_DIR}/src/common/serial.hpp
        ${CMAKE_SOURCE_DIR}/src/common/serial.cpp
        ${CMAKE_SOURCE_DIR}/src/common/serial_lever.hpp
        ${CMAKE_SOURCE_DIR}/src/common/serial_lever.cpp
        ${CMAKE_SOURCE_DIR}/src/common/juice_pump.hpp
        ${CMAKE_SOURCE_DIR}/src/common/juice_pump.cpp
        ${CMAKE_SOURCE_DIR}/src/common/juice_pump_gui.hpp
        ${CMAKE_SOURCE_DIR}/src/common/juice_pump_gui.cpp
        ${CMAKE_SOURCE_DIR}/src/common/lever_system.hpp
        ${CMAKE_SOURCE_DIR}/src/common/lever_system.cpp
        ${CMAKE_SOURCE_DIR}/src/common/lever_pull.hpp
        ${CMAKE_SOURCE_DIR}/src/common/lever_pull.cpp
        ${CMAKE_SOURCE_DIR}/src/common/om.hpp
        ${CMAKE_SOURCE_DIR}/src/common/lever_gui.hpp
        ${CMAKE_SOURCE_DIR}/src/common/lever_gui.cpp
        ${CMAKE_SOURCE_DIR}/src/common/glfw.hpp
        ${CMAKE_SOURCE_DIR}/src/common/glfw.cpp
        ${CMAKE_SOURCE_DIR}/src/common/imgui.hpp
        ${CMAKE_SOURCE_DIR}/src/common/imgui.cpp
        ${CMAKE_SOURCE_DIR}/src/common/identifier.hpp
        ${CMAKE_SOURCE_DIR}/src/common/ringbuffer.hpp
        ${CMAKE_SOURCE_DIR}/src/common/handshake.hpp
        ${CMAKE_SOURCE_DIR}/src/common/vector.hpp
        ${CMAKE_SOURCE_DIR}/src/common/random.hpp
        ${CMAKE_SOURCE_DIR}/src/common/random.cpp
        ${CMAKE_SOURCE_DIR}/src/common/render.hpp
        ${CMAKE_SOURCE_DIR}/src/common/render.cpp
        ${CMAKE_SOURCE_DIR}/src/common/time.hpp
        ${CMAKE_SOURCE_DIR}/src/common/time.cpp

        #   GLAD
        ${CMAKE_SOURCE_DIR}/deps/glad/src/glad.c

        #   IMGUI
        ${CMAKE_SOURCE_DIR}/deps/imgui/imgui.cpp
        ${CMAKE_SOURCE_DIR}/deps/imgui/imgui_draw.cpp
        ${CMAKE_SOURCE_DIR}/deps/imgui/imgui_tables.cpp
        ${CMAKE_SOURCE_DIR}/deps/imgui/imgui_widgets.cpp
        ${CMAKE_SOURCE_DIR}/deps/imgui/imgui_demo.cpp
        ${CMAKE_SOURCE_DIR}/deps/imgui/backends/imgui_impl_glfw.cpp
        ${CMAKE_SOURCE_DIR}/deps/imgui/backends/imgui_impl_opengl3.cpp

        #   IMPLOT
        ${CMAKE_SOURCE_DIR}/deps/implot/implot.cpp
        ${CMAKE_SOURCE_DIR}/deps/implot/implot_items.cpp)

configure_compiler_flags(${PROJECT_NAME})

if (MSVC)
    link_directories(lib/win/${CMAKE_BUILD_TYPE})
else()
    add_subdirectory(deps/serial)
endif()

add_subdirectory(deps/glfw)
add_subdirectory(deps/portaudio)

target_link_libraries(${PROJECT_NAME} PUBLIC
        glfw
        PortAudio
        serial
        NIDAQmx)

add_subdirectory(src/sandbox)