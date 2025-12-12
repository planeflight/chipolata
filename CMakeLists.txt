cmake_minimum_required(VERSION 3.5)

project(chipolata)

# autocomplete
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

SET (CMAKE_CXX_FLAGS "-Wall -Wextra -Wcast-qual -Wconversion-null -Wformat-security \
-Wmissing-declarations -Woverlength-strings -Wpointer-arith -Wundef \
-Wunused-local-typedefs -Wunused-result -Wvarargs -Wvla \
-Wwrite-strings -DNOMINMAX -fno-omit-frame-pointer -fPIC -g")

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(lib/SDL)
add_subdirectory(lib/SDL_image)

set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)

add_executable(${PROJECT_NAME}
    src/main.cpp
)


include(FindPkgConfig)

include(FetchContent)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.15.3
)
set(SPDLOG_INSTALL OFF)
set(SPDLOG_BUILD_SHARED OFF)
FetchContent_MakeAvailable(spdlog)

target_link_libraries(${PROJECT_NAME}
    SDL3::SDL3
    SDL3_image::SDL3_image
    spdlog::spdlog
)
target_include_directories(${PROJECT_NAME} PUBLIC
    lib/SDL/include/
)
