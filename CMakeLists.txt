cmake_minimum_required(VERSION 2.8.3)

project(voxel_playground)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
list(APPEND DEPENDENCIES ${OPENGL_LIBRARIES})

add_definitions(-DELPP_THREAD_SAFE)
add_definitions(-DNOMINMAX)

set(SHADERS resources/shaders/fragment.glsl
            resources/shaders/vertex.glsl
            resources/shaders/arrow_fragment.glsl
            resources/shaders/arrow_vertex.glsl)

if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    set(RESOURCES resources/shaders/fragment.glsl
                  resources/shaders/vertex.glsl
                  resources/shaders/arrow_fragment.glsl
                  resources/shaders/arrow_vertex.glsl
                  resources/mish.jpg
                  resources/mish_dark.jpg
                  resources/white.png
                  resources/dragon.obj
                  resources/earth.obj
                  resources/Diffuse_2K.png
                  resources/Night_lights_2K.png
                  resources/Normal_2K.png
                  resources/Ocean_Mask_2K.png
                  resources/cat_diff.tga
                  resources/cat.obj
                  resources/cat.mtl
                  resources/cube.obj
                  resources/normal_map.png
                  )

    foreach(item IN LISTS RESOURCES)
        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${item}"
            COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${item}" "${CMAKE_CURRENT_BINARY_DIR}/${item}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${item}"
        )
    endforeach()
endif()
add_custom_target(resources-target ALL DEPENDS ${RESOURCES})

if(WIN32)
    add_subdirectory(third-party/glfw)
    list(APPEND DEPENDENCIES glfw3)
else()
    find_path(GLFW_INCLUDE_DIR NAMES GLFW/glfw3.h
        PATHS /usr/X11R6/include
              /usr/include/X11
              /opt/graphics/OpenGL/include
              /opt/graphics/OpenGL/contrib/libglfw
              /usr/local/include
              /usr/include/GL
              /usr/include
    )
    find_library(GLFW_LIBRARIES NAMES glfw glfw3
            PATHS /usr/lib64
                  /usr/lib
                  /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}
                  /usr/local/lib64
                  /usr/local/lib
                  /usr/local/lib/${CMAKE_LIBRARY_ARCHITECTURE}
                  /usr/X11R6/lib
    )
    list(APPEND DEPENDENCIES ${GLFW_LIBRARIES})
    include_directories(${GLFW_INCLUDE_DIR})
endif()

add_executable(voxel-playground 
    src/voxel-playground.cpp 
    src/shader.cpp src/shader.h 
    src/window.cpp src/window.h 
    src/texture.cpp src/texture.h 
    src/util.cpp src/util.h 
    src/vao.cpp src/vao.h 
    src/vbo.cpp src/vbo.h 
    src/camera.cpp src/camera.h 
    src/model.cpp src/model.h
    src/loader.cpp src/loader.h
    third-party/imgui/imgui.cpp 
    third-party/imgui/imgui_draw.cpp 
    third-party/imgui/imgui_impl_glfw.cpp
    third-party/imgui/imgui_impl_opengl3.cpp
    third-party/easyloggingpp/easylogging++.cc
    third-party/gl3w/src/gl3w.c
    ${SHADERS}
    )
include_directories(voxel-playground third-party/imgui third-party/gl3w/include 
                    third-party/easyloggingpp third-party third-party/readerwriterqueue
                    third-party/linalg third-party/OBJ-Loader)
target_link_libraries(voxel-playground ${DEPENDENCIES})
add_dependencies(voxel-playground resources-target)

if(WIN32)
    source_group("Source Files" FILES
        third-party/voxel-playground.cpp)

    source_group("3rd-Party" FILES
        third-party/imgui/imgui.cpp 
        third-party/imgui/imgui_impl_glfw.cpp
        third-party/imgui/imgui_impl_opengl3.cpp
        third-party/imgui/imgui_draw.cpp 
        third-party/easyloggingpp/easylogging++.cc
        third-party/gl3w/src/gl3w.c
    )

    source_group("Shaders" FILES
        ${SHADERS}
    )

    set_target_properties (glfw3 PROPERTIES
        FOLDER Third-Party
    )
endif()
