set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

if (MSVC)
	add_compile_definitions(MSVC=1)
	# disable warnings
	add_compile_options(
		/wd4820 /wd4668 /wd4255 /wd4100 /wd4996 /wd5045 /wd4061 /wd4206 /wd4295 /wd4132 /wd4701 /wd4702
		/wd4191 /wd4242 /wd4389 /wd4018 /wd4293 /wd4388 /wd4267 /wd4244 /wd4201 /wd4710 /wd4189 /wd4711
	)
	# remove unused code
	add_compile_options(/Gy)
else ()
	set(CMAKE_C_STANDARD 11)
	# disable warnings
	if (APPLE)
		add_compile_options(-Wno-attributes -Wno-deprecated-declarations)
	else ()
		add_compile_options(-Wno-attributes -Wno-dangling-pointer)
	endif ()
	# remove unused code
	add_compile_options(-fdata-sections -ffunction-sections)
	if (NOT APPLE AND NOT WIN32)
		add_link_options(-Wl,--gc-sections)
	endif ()
endif ()

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
if (UNIX OR MSVC)
	set(BUILD_SHARED_LIBS OFF)
endif ()
