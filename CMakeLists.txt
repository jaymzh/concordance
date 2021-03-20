cmake_minimum_required(VERSION 3.19)
project(concordance)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_subdirectory(concordance)
add_subdirectory(libconcord)
