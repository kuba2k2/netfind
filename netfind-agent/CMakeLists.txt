cmake_minimum_required(VERSION 3.20)

project(netfind-agent C)

set(CMAKE_POLICY_VERSION_MINIMUM 3.10)

include(cmake/CompilerOptions.cmake)
include(cmake/GitVersion.cmake)
include(cmake/PrintAllTargets.cmake)

add_subdirectory("lib/")
add_subdirectory("proto/")
add_subdirectory("src/")

print_all_targets("${CMAKE_CURRENT_LIST_DIR}")
