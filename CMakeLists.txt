cmake_minimum_required(VERSION 3.14)
project(thread_pool LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


add_library(thread_pool INTERFACE)
target_include_directories(thread_pool INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)


add_executable(demo src/main.cpp)
target_link_libraries(demo PRIVATE thread_pool)


add_executable(smoke tests/smoke.cpp)
target_link_libraries(smoke PRIVATE thread_pool)