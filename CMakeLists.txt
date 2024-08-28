cmake_minimum_required(VERSION 3.15)
project(raydar)

set(RAYLIB_VERSION 5.0)

find_package(raylib ${RAYLIB_VERSION} QUIET)
if (NOT raylib_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED)
    set(FETCHCONTENT_QUIET NO)
    FetchContent_MakeAvailable(raylib)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  endif()
endif()

set(CMAKE_C_STANDARD 11)

add_executable(
  ${PROJECT_NAME}
  src/main.c
)

target_link_libraries(${PROJECT_NAME} raylib)

# Make main find the <raylib.h> header (and others)
target_include_directories(${PROJECT_NAME} PUBLIC "${raylib_SOURCE_DIR}/src")
