cmake_minimum_required (VERSION 3.10)
project (v8unpack)

set (v8unpack_VERSION_MAJOR 3)
set (v8unpack_VERSION_MINOR 0)

if(WIN32)
	set(RC_FILE "VersionInfo.rc")
else()
	set(RC_FILE "")
endif()

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

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

# По умолчанию используем динамический Boost (работает на Linux/Ubuntu)
option(USE_STATIC_BOOST "Link against static boost libraries" OFF)
if(USE_STATIC_BOOST)
	set (Boost_USE_STATIC_LIBS ON)
	set (Boost_USE_MULTITHREADED OFF)
	set (Boost_USE_STATIC_RUNTIME ON)
else()
	set (Boost_USE_STATIC_LIBS OFF)
	set (Boost_USE_STATIC_RUNTIME OFF)
	set (Boost_USE_MULTITHREADED ON)
endif()

find_package (Boost 1.53 REQUIRED COMPONENTS filesystem system iostreams)


include_directories (${Boost_INCLUDE_DIRS})
target_link_libraries (v8unpack ${Boost_LIBRARIES})

find_package (ZLIB REQUIRED)

include_directories (${ZLIB_INCLUDE_DIRS})
target_link_libraries (v8unpack ${ZLIB_LIBRARIES})
target_link_libraries (v8unpack Qt6::Core)   # <-- добавьте это

#target_link_libraries(v8unpack PRIVATE Qt6::Core)