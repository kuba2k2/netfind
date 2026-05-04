set(OUTPUT_INCLUDE "${CMAKE_BINARY_DIR}/git_version")
set(OUTPUT_HEADER "${OUTPUT_INCLUDE}/git_version.h")

add_custom_command(
	OUTPUT ${OUTPUT_HEADER}
	COMMAND ${CMAKE_COMMAND}
	ARGS
	-D SRC=${CMAKE_CURRENT_LIST_DIR}/git_version.h.in
	-D DST=${OUTPUT_HEADER}
	-P ${CMAKE_CURRENT_LIST_DIR}/GitVersionGenerate.cmake
	COMMENT "Generating git_version.h"
)

add_custom_target(git_version_generate DEPENDS ${OUTPUT_HEADER})
add_library(git_version INTERFACE)
add_dependencies(git_version git_version_generate)
target_include_directories(git_version INTERFACE "${OUTPUT_INCLUDE}")
