cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(Engine)

set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")

configure_file(
	${PROJECT_SOURCE_DIR}/config.h.in
	${PROJECT_BINARY_DIR}/config.h
)

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
	source/graphics/Texture.h
	source/graphics/Texture.cpp
	source/render/Material.h
	source/render/Material.cpp
	source/render/Mesh.h
	source/render/Mesh.cpp
	source/render/RenderQueue.h
	source/render/RenderQueue.cpp
	source/scene/GameObject.h
	source/scene/GameObject.cpp
	source/scene/Scene.h
	source/scene/Scene.cpp
	source/scene/Component.h
	source/scene/Component.cpp
	source/scene/components/MeshComponent.h
	source/scene/components/MeshComponent.cpp
	source/scene/components/CameraComponent.h
	source/scene/components/CameraComponent.cpp
	source/scene/components/PlayerControllerComponent.h
	source/scene/components/PlayerControllerComponent.cpp
	source/scene/components/LightComponent.h
	source/scene/components/LightComponent.cpp
	source/scene/components/AnimationComponent.h
	source/scene/components/AnimationComponent.cpp
	source/io/FileSystem.h
	source/io/FileSystem.cpp
	source/Common.h
	${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glad/src/glad.c
)

# Group files according to files organization
foreach(FILE ${PROJECT_SOURCE_FILES}) 
    # Get the directory of the source file
    get_filename_component(PARENT_DIR "${FILE}" DIRECTORY)

    # Remove common directory prefix to make the group
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" GROUP "${PARENT_DIR}")

    # Make sure we are using windows slashes
    string(REPLACE "/" "\\" GROUP "${GROUP}")

    source_group("${GROUP}" FILES "${FILE}")
endforeach()

include_directories(source)
add_library(${PROJECT_NAME} ${PROJECT_SOURCE_FILES})

# Add GLFW library
add_subdirectory(thirdparty/glfw-3.4 "${CMAKE_CURRENT_BINARY_DIR}/glfw_build")
include_directories(thirdparty/glfw-3.4/include)

# 添加glad头文件路径
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glad/include
)

# Add GLM library
add_subdirectory(thirdparty/glm-1.0.3/glm)
include_directories(thirdparty/glm-1.0.3)

include_directories(thirdparty/stb_image)

include_directories(thirdparty/json)

include_directories(thirdparty/cgltf)

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_BINARY_DIR})

# Link all thirdparty libraries
target_link_libraries(${PROJECT_NAME} 
    glfw
	glm
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${ASSETS_DIR}
	${CMAKE_BINARY_DIR}/assets
)