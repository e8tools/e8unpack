cmake_minimum_required (VERSION 3.10)
project (v8unpack)

set (v8unpack_VERSION_MAJOR 3)
set (v8unpack_VERSION_MINOR 0)

if(WIN32)
	set(RC_FILE "VersionInfo.rc")
else()
	set(RC_FILE "")
endif()

if(NOT MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

foreach(stale_define V8_DEFAULT_PAGE_SIZE V8_OFFSET_8316 V8_FF_SIGNATURE V8_FF64_SIGNATURE)
	if(CMAKE_CXX_FLAGS MATCHES "${stale_define}")
		message(WARNING "Уберите -D${stale_define} из CMAKE_CXX_FLAGS: это имя занято константой в src/V8File.h.")
	endif()
endforeach()

set (V8UNPACK_SOURCES src/main.cpp src/V8File.cpp src/utils.cpp src/placeholder216.cpp src/VersionFile.cpp)
set (V8UNPACK_HEADERS src/V8File.h src/version.h src/zconf.h src/zlib.h src/VersionFile.h)

if((MSVC) AND (NOT DISABLE_SINGLE_FILE))

	foreach(flag_var
			CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
			CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)

		if(${flag_var} MATCHES "/MD")
			string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
		endif(${flag_var} MATCHES "/MD")

	endforeach(flag_var)

	set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB:MSVCRT")
	set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:MSVCRT")
	set (CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:MSVCRTD")

endif()

set(CMAKE_CXX_STANDARD 14)

find_package (Qt6 REQUIRED COMPONENTS Core)

add_executable (v8unpack ${V8UNPACK_SOURCES} ${V8UNPACK_HEADERS} ${RC_FILE}
	src/core/MetadataTypes.h
	src/core/MetadataTypes.cpp
	src/parser/Parse_tree.h
	src/parser/Parse_tree.cpp
	src/parser/NodeTypes.h
	src/metadata/ConfigStructureReader.h
	src/metadata/ConfigStructureReader.cpp
	src/metadata/SectionTypes.h
	src/metadata/SectionTypes.cpp
	src/core/V8Container.h
	src/core/V8FileHeader.h
	src/core/V8Container.cpp
	src/core/V8FileHeader.cpp
	src/core/V8Unpacker.h
	src/core/V8Unpacker.cpp
	src/metadata/MetadataMap.h
	src/metadata/MetadataMap.cpp
	src/metadata/MetadataMapper.h
	src/metadata/MetadataMapper.cpp
	src/output/FileLayoutBuilder.h
	src/output/FileLayoutBuilder.cpp
	src/output/StructuredPacker.h
	src/output/StructuredPacker.cpp
	src/output/StructuredUnpacker.h
	src/output/UnpackManifest.h
	src/output/StructuredUnpacker.cpp
	src/output/UnpackManifest.cpp)

target_include_directories(v8unpack PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)	

add_definitions (-DBOOST_ALL_NO_LIB)

# По умолчанию линкуем статический Boost: иначе рядом с v8unpack
# требуются boost_*.so / boost_*.dll. Динамический вариант — USE_STATIC_BOOST=OFF.
option(USE_STATIC_BOOST "Link against static boost libraries" ON)
option(Boost_USE_MULTITHREADED "Link against multithreaded boost libraries" ON)

if(USE_STATIC_BOOST)
	set (Boost_USE_STATIC_LIBS ON)
	if(MSVC)
		set (Boost_USE_STATIC_RUNTIME ON)
	else()
		set (Boost_USE_STATIC_RUNTIME OFF)
	endif()
else()
	set (Boost_USE_STATIC_LIBS OFF)
	set (Boost_USE_STATIC_RUNTIME OFF)
endif()

# Подсказки для поиска Boost (можно задать в кэше через -D):
#   Boost_ROOT / BOOST_ROOT — корень Boost (каталог с boost/ и lib*/);
#       поддерживаются оба написания: boost-cmake ищет по Boost_ROOT,
#       а модуль FindBoost — по BOOST_ROOT и падает от чужого написания;
#   Boost_LIBRARY_DIR / BOOST_LIBRARYDIR — каталог с собранными библиотеками;
#   Boost_COMPILER    — суффикс тулсета, если библиотеки собраны другим MSVC
#       (например, -vc142 для сборок VS2019, используемых в VS2022).
set (Boost_ROOT "" CACHE PATH "Boost root directory (also BOOST_ROOT)")
set (BOOST_ROOT "" CACHE PATH "Boost root directory")
set (Boost_LIBRARY_DIR "" CACHE PATH "Boost libraries directory")
set (BOOST_LIBRARYDIR "" CACHE PATH "Boost libraries directory")

if(Boost_ROOT)
	list (APPEND CMAKE_PREFIX_PATH "${Boost_ROOT}")
elseif(BOOST_ROOT)
	list (APPEND CMAKE_PREFIX_PATH "${BOOST_ROOT}")
endif()

if(NOT Boost_LIBRARY_DIR)
	if(BOOST_LIBRARYDIR)
		set (Boost_LIBRARY_DIR "${BOOST_LIBRARYDIR}")
	endif()
endif()

if(Boost_LIBRARY_DIR)
	set (BOOST_LIBRARYDIR "${Boost_LIBRARY_DIR}" CACHE PATH "Boost libraries directory" FORCE)
endif()

set (Boost_COMPILER "" CACHE STRING "Boost toolset suffix, e.g. -vc142")

find_package (Boost 1.53 REQUIRED COMPONENTS filesystem system iostreams)


include_directories (${Boost_INCLUDE_DIRS})
target_link_libraries (v8unpack ${Boost_LIBRARIES})

find_package (ZLIB REQUIRED)

include_directories (${ZLIB_INCLUDE_DIRS})
target_link_libraries (v8unpack ${ZLIB_LIBRARIES})
target_link_libraries (v8unpack Qt6::Core)   # <-- добавьте это

#target_link_libraries(v8unpack PRIVATE Qt6::Core)
