cmake_minimum_required(VERSION 3.20)

if (NOT BUILD_SHARED_LIBS)
	# If linking statically
	if (MSVC)
		# ...and if on MSVC
		set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	endif ()
endif ()

# lt_logger
add_subdirectory(lt_logger)

# uthash
add_library(uthash INTERFACE)
target_include_directories(uthash INTERFACE "uthash/src/")

# printf
add_library(printf STATIC "printf/src/printf/printf.c")
target_compile_definitions(printf PRIVATE -DPRINTF_IMPLEMENTATION=1)
target_compile_definitions(printf PUBLIC -DPRINTF_INCLUDE_CONFIG_H=1)
target_include_directories(printf PUBLIC "printf/src/" "printf_config/")
target_link_options(printf PUBLIC "-Wl,-wrap,printf" "-Wl,-wrap,vprintf")
target_link_options(printf PUBLIC "-Wl,-wrap,sprintf" "-Wl,-wrap,vsprintf")
target_link_options(printf PUBLIC "-Wl,-wrap,snprintf" "-Wl,-wrap,vsnprintf")

# cJSON
set(ENABLE_CJSON_UNINSTALL OFF)
set(ENABLE_CJSON_TEST OFF)
set(ENABLE_TARGET_EXPORT OFF)
set(BUILD_SHARED_AND_STATIC_LIBS ON)
set(ENABLE_PUBLIC_SYMBOLS OFF)
set(ENABLE_HIDDEN_SYMBOLS ON)
add_subdirectory(cjson)
target_include_directories(cjson-static INTERFACE "cjson/")
if (NOT MSVC)
	target_compile_options(cjson-static PRIVATE -Wno-float-conversion)
endif ()

# libpcap
set(DISABLE_AIRPCAP ON)
set(DISABLE_DAG ON)
set(DISABLE_DBUS ON)
set(DISABLE_RDMA ON)
set(DISABLE_SEPTEL ON)
set(DISABLE_SNF ON)
set(DISABLE_TC ON)
set(ENABLE_REMOTE OFF)
add_subdirectory(libpcap)
if (WIN32)
	# Workaround for MSYS2/Mingw64 not having this macro in <sal.h>
	target_compile_definitions(pcap_static PUBLIC -D_Post_invalid_=)
	# Link with Packet.lib from Npcap SDK - don't know of any better way
	target_link_libraries(pcap_static PUBLIC "${Packet_ROOT}/Lib/x64/Packet.lib")
endif ()
