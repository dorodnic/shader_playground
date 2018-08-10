cmake_minimum_required(VERSION 2.8.3)

project(voxel_playground)

find_package(OpenGL REQUIRED)
list(APPEND DEPENDENCIES ${OPENGL_LIBRARIES})

if(WIN32)
	add_subdirectory(glfw)
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
	voxel-playground.cpp 
	imgui/imgui.cpp 
	imgui/imgui_impl_glfw.cpp)
include_directories(voxel-playground imgui)
target_link_libraries(voxel-playground ${DEPENDENCIES})

if(WIN32)
	source_group("Source Files" FILES
		voxel-playground.cpp)
endif()