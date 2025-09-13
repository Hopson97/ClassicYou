cmake_minimum_required(VERSION 3.10)

project(
    classic-you 
    VERSION 1.0
)

include("$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/GUI.cpp
    src/main.cpp

    src/Editor/Actions.cpp
    src/Editor/EditorGUI.cpp
    src/Editor/EditorLevel.cpp
    src/Editor/FloorManager.cpp
    src/Editor/LegacyFileConverter.cpp
    src/Editor/LevelObject.cpp
    src/Editor/Tool.cpp

    src/Graphics/Camera.cpp
    src/Graphics/CameraController.cpp
    src/Graphics/Mesh.cpp
    #src/Graphics/Model.cpp
    src/Graphics/ShadowMap.cpp
    src/Graphics/Skybox.cpp
    
    src/Graphics/OpenGL/Framebuffer.cpp
    src/Graphics/OpenGL/GLUtils.cpp
    src/Graphics/OpenGL/Shader.cpp
    src/Graphics/OpenGL/Texture.cpp
    src/Graphics/OpenGL/VertexArrayObject.cpp

    src/Screens/Screen.cpp
    src/Screens/ScreenEditGame.cpp
    src/Screens/ScreenMainMenu.cpp
    src/Screens/ScreenPlaying.cpp

    src/Util/ImGuiExtras.cpp
    src/Util/Keyboard.cpp
    src/Util/Maths.cpp
    src/Util/Profiler.cpp
    src/Util/Util.cpp
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)

target_compile_definitions(${PROJECT_NAME} PRIVATE GLM_ENABLE_EXPERIMENTAL)

if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
    set(CMAKE_CXX_FLAGS_RELEASE "/Ox")
  	target_compile_options(${PROJECT_NAME} PRIVATE /W4 _CRT_SECURE_NO_WARNINGS)
else()
  	target_compile_options(${PROJECT_NAME} PRIVATE 
		-Wall -Wextra -pedantic)		
endif()

find_package(SFML COMPONENTS Network Graphics Window Audio System CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(ctre CONFIG REQUIRED)

add_subdirectory(deps)
target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
    deps
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SFML::Network SFML::Graphics SFML::Window SFML::Audio SFML::System
    glm::glm
    imgui::imgui
    ctre::ctre
    imgui_sfml
    glad 
)
