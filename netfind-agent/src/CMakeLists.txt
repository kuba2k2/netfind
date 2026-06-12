cmake_minimum_required(VERSION 3.20)

# Sources
file(GLOB_RECURSE SOURCES "*.c" "conn/*.c" "devdb/*.c" "module/*.c" "module/**/*.c")
add_executable(${PROJECT_NAME} ${SOURCES})

# Compiler options
target_compile_options(${PROJECT_NAME} PUBLIC -Wall)
target_compile_definitions(${PROJECT_NAME} PUBLIC CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
target_include_directories(${PROJECT_NAME} PUBLIC ".")

# Common link libraries
target_link_libraries(${PROJECT_NAME} PUBLIC cjson-static)
target_link_libraries(${PROJECT_NAME} PUBLIC git_version)
target_link_libraries(${PROJECT_NAME} PUBLIC lt_logger)
target_link_libraries(${PROJECT_NAME} PUBLIC pcap_static)
target_link_libraries(${PROJECT_NAME} PUBLIC printf)
target_link_libraries(${PROJECT_NAME} PUBLIC uthash)
target_link_libraries(${PROJECT_NAME} PUBLIC libcurl_static)
target_link_libraries(${PROJECT_NAME} PUBLIC proto)

# Link libraries for various platforms
if (WIN32)
	target_link_libraries(${PROJECT_NAME} PUBLIC ws2_32 iphlpapi)
else ()
	target_link_libraries(${PROJECT_NAME} PUBLIC m dl)
endif ()

# Configure release builds
if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
	if (MSVC)
		target_link_options(${PROJECT_NAME} PUBLIC /SUBSYSTEM:WINDOWS)
	elseif (WIN32)
		target_link_options(${PROJECT_NAME} PUBLIC -mwindows)
	else ()
		target_link_options(${PROJECT_NAME} PUBLIC -static-libgcc)
	endif ()
endif ()
