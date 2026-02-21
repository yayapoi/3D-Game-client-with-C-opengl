cmake_minimum_required(VERSION 3.10)

project(Engine)

set(PROJECT_SOURCE_FILES
	source/Engine.h
	source/Engine.cpp
	source/Application.h
	source/Application.cpp
	source/input/InputManager.h
	source/input/InputManager.cpp
	source/graphics/ShaderProgram.h
	source/graphics/ShaderProgram.cpp
	source/graphics/GraphicsAPI.h
	source/graphics/GraphicsAPI.cpp
	source/render/Material.h
	source/render/Material.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glad/src/glad.c
)

include_directories(source)
add_library(${PROJECT_NAME} ${PROJECT_SOURCE_FILES})

# Add GLFW library
add_subdirectory(thirdparty/glfw-3.4 "${CMAKE_CURRENT_BINARY_DIR}/glfw_build")
include_directories(thirdparty/glfw-3.4/include)

# 添加glad头文件路径
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glad/include
)

# Link all thirdparty libraries
target_link_libraries(${PROJECT_NAME} 
    glfw 
)