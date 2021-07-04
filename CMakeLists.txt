cmake_minimum_required(VERSION 3.15)
project(star)

set(CMAKE_CXX_STANDARD 14)
add_definitions(-D NOMINMAX)

include_directories(Source)
add_library(cgltf INTERFACE)
target_include_directories(cgltf INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/cgltf/include)

include(Source/Sources.cmake)
add_subdirectory(GearEngine)
add_executable(star main.cpp ${STAR_SRC})
target_link_libraries(star GearEngine)
target_link_libraries(star vulkan)
target_link_libraries(star glfw)
target_link_libraries(star glslang)
target_link_libraries(star spirv_cross)
target_link_libraries(star cgltf)

# builtin resources
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Resources DESTINATION ${CMAKE_INSTALL_PREFIX})