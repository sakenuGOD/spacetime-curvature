cmake_minimum_required(VERSION 3.15)
project(spacetime-curvature CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

add_executable(spacetime-curvature main.cpp)
target_link_libraries(spacetime-curvature PRIVATE OpenGL::GL GLEW::GLEW glfw)

if(WIN32)
    add_custom_command(TARGET spacetime-curvature POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:spacetime-curvature>
            $<TARGET_FILE_DIR:spacetime-curvature>
        COMMAND_EXPAND_LISTS)
endif()
