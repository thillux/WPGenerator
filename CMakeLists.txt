cmake_minimum_required(VERSION 2.8)
project(WPGenerator)

FILE(GLOB Sources *.c)

SET(CMAKE_MODULE_PATH "CMake")

IF(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(RESSOURCEDIR "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}")
ELSE()
  set(RESSOURCEDIR "${CMAKE_SOURCE_DIR}/res")
ENDIF(CMAKE_BUILD_TYPE STREQUAL "Release")
add_definitions("-D DATAROOTDIR=${RESSOURCEDIR}")

add_definitions("-O2 -std=c99 -march=native -g -Wall -Wextra -pedantic")

add_executable(${PROJECT_NAME} ${Sources})

find_package (RSVG)
if (RSVG_FOUND)
  include_directories(${RSVG_INCLUDE_DIRS})
  target_link_libraries (${PROJECT_NAME} ${RSVG_LIBRARIES})

  include_directories(${CAIRO_INCLUDE_DIRS})
  target_link_libraries (${PROJECT_NAME} ${CAIRO_LIBRARIES})
endif (RSVG_FOUND)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(FILES "res/archlinux-logo-light-scalable.svg" DESTINATION "share/${PROJECT_NAME}")
