project(dfusepp)
cmake_minimum_required(VERSION 3.1)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE include)

if (BUILD_EXAMPLE)
    add_executable(${PROJECT_NAME}-example src/example/main.cpp)
    target_include_directories(${PROJECT_NAME}-example PUBLIC include)
endif()

if (BUILD_TEST)
    add_subdirectory(src)
    add_subdirectory(test)
endif()
