cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

get_filename_component(ProjectName ${CMAKE_CURRENT_LIST_DIR} NAME)

project(${ProjectName})

file(GLOB LOCAL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")

add_compile_options(-Wconversion)

set(SOURCES ${LOCAL_SOURCES})

add_executable(${PROJECT_NAME} ${SOURCES})