cmake_minimum_required(VERSION 3.22)
project(AstroDrift LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(BUILD_SHARED_LIBS OFF)

FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.1
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM
)

FetchContent_MakeAvailable(SFML)

add_executable(AstroDrift main.cpp)

target_link_libraries(AstroDrift
    PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
        SFML::Audio
)