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

# nanopb
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/nanopb/extra/")
find_package(Nanopb REQUIRED)

# wolfSSL
set(WOLFSSL_CURL ON)
set(WOLFSSL_EXAMPLES OFF)
set(WOLFSSL_CRYPT_TESTS OFF)
set(WOLFSSL_INSTALL OFF)
# force linking wolfSSL statically
set(WOLFSSL_WAS_SHARED BUILD_SHARED_LIBS)
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(wolfssl)
set(BUILD_SHARED_LIBS WOLFSSL_WAS_SHARED)
# make curl's FindWolfSSL detect the just-built library
set(WOLFSSL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/wolfssl/")
set(WOLFSSL_LIBRARY wolfssl)

# curl
set(BUILD_CURL_EXE OFF)
set(BUILD_STATIC_LIBS ON)
set(BUILD_LIBCURL_DOCS OFF)
set(BUILD_MISC_DOCS OFF)
set(CURL_DISABLE_INSTALL ON)
set(CURL_DISABLE_ALTSVC ON)
set(CURL_DISABLE_SRP ON)
set(CURL_DISABLE_COOKIES ON)
set(CURL_DISABLE_BASIC_AUTH ON)
set(CURL_DISABLE_DIGEST_AUTH ON)
set(CURL_DISABLE_KERBEROS_AUTH ON)
set(CURL_DISABLE_NEGOTIATE_AUTH ON)
set(CURL_DISABLE_AWS ON)
set(CURL_DISABLE_DICT ON)
set(CURL_DISABLE_DOH ON)
set(CURL_DISABLE_FILE ON)
set(CURL_DISABLE_FTP ON)
set(CURL_DISABLE_GOPHER ON)
set(CURL_DISABLE_IMAP ON)
set(CURL_DISABLE_LDAP ON)
set(CURL_DISABLE_MIME ON)
set(CURL_DISABLE_FORM_API ON)
set(CURL_DISABLE_MQTT ON)
set(CURL_DISABLE_BINDLOCAL ON)
set(CURL_DISABLE_NETRC ON)
set(CURL_DISABLE_PARSEDATE ON)
set(CURL_DISABLE_POP3 ON)
set(CURL_DISABLE_PROGRESS_METER ON)
set(CURL_DISABLE_IPFS ON)
set(CURL_DISABLE_RTSP ON)
set(CURL_DISABLE_SHA512_256 ON)
set(CURL_DISABLE_SHUFFLE_DNS ON)
set(CURL_DISABLE_SMTP ON)
set(CURL_DISABLE_SOCKETPAIR ON)
set(CURL_DISABLE_TELNET ON)
set(CURL_DISABLE_TFTP ON)
set(CURL_USE_LIBPSL OFF)
set(CURL_USE_LIBSSH2 OFF)
set(CURL_USE_OPENSSL OFF)
set(CURL_USE_WOLFSSL ON)
add_subdirectory(curl)
