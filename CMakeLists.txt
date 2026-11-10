cmake_minimum_required(VERSION 3.24)
project(mygame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-3.2.8   # check for the latest 3.2.x/3.x release tag
)
FetchContent_MakeAvailable(SDL3)

 include(FetchContent)
FetchContent_Declare(json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
     ${CMAKE_SOURCE_DIR}/src/*.cpp
     ${CMAKE_SOURCE_DIR}/src/*.h)

add_executable(mygame ${SOURCES})
target_include_directories(mygame PRIVATE src)
target_link_libraries(mygame PRIVATE SDL3::SDL3 nlohmann_json::nlohmann_json)

add_custom_command(TARGET mygame POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_FILE:SDL3::SDL3-shared> $<TARGET_FILE_DIR:mygame>
  VERBATIM)