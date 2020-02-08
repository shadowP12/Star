cmake_minimum_required(VERSION 3.15)
project(star)

set(CMAKE_CXX_STANDARD 14)

include_directories(Star)
include_directories(ThirdParty/glfw/include)
include_directories(ThirdParty/opencl/include)
include_directories(ThirdParty/glm/include)
include_directories(ThirdParty/stb)
include_directories(ThirdParty/cgltf)

add_executable(star
        Star/main.cpp
        Star/Cl/RendererCL.cpp
        Star/Common/SceneData.cpp
        Star/common/Window.cpp
        Star/GLAD/glad.c
        Star/Tools/Tools.cpp
        Star/BVH.cpp
        Star/BVH.h
        Star/Camera.cpp
        Star/Camera.h
        Star/DebugDraw.h
        Star/Model.cpp
        Star/Model.h
        Star/Node.cpp
        Star/Node.h
        Star/Primitive.cpp
        Star/Primitive.h
        Star/Ray.h
        Star/Reflection.h
        Star/Scene.cpp
        Star/Scene.h
        Star/ShaderProgram.h
        Star/Triangle.cpp
        Star/Triangle.h
        Star/TriangleMesh.cpp
)

target_link_libraries(star ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/opencl/lib/win32/OpenCL.lib)

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    target_link_libraries(star ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glfw/lib/win32/debug/glfw3.lib)
else()
    target_link_libraries(star ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glfw/lib/win32/release/glfw3.lib)
endif()

set(CMAKE_INSTALL_PREFIX "./")
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Kernels DESTINATION ${CMAKE_INSTALL_PREFIX})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Res DESTINATION ${CMAKE_INSTALL_PREFIX})


