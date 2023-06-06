cmake_minimum_required(VERSION 3.0)

set(CMAKE_C_COMPILER "C:/Program Files/w64devkit/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "C:/Program Files/w64devkit/bin/g++.exe")

project(path_tracer)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(glm_DIR "C:/Program Files/GLM/lib/cmake/glm")
set(glfw3_DIR "C:/Program Files/GLFW/lib/cmake/glfw3")

# Add GLFW and GLM
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# Include directories
include_directories(include ${GLM_INCLUDE_DIRS})

# Create the executable
file(GLOB SOURCES "src/*.cpp" "src/*.c")
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} glfw)