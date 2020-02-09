cmake_minimum_required (VERSION 3.5)
project (dcdm2imf)

# import boost

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIR})

# import asdcplib

add_subdirectory(lib/asdcplib)
include_directories(lib/asdcplib/src)

if(WIN32)
	add_definitions(/DKM_WIN32 /D_CONSOLE /DASDCP_PLATFORM=\"win32\" /D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_WARNINGS)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
elseif(UNIX)
	add_definitions(/DASDCP_PLATFORM=\"unix\")
endif(WIN32)

# project files

file(GLOB_RECURSE SRC_FILES src/main/*.cpp src/*.h )
file(GLOB_RECURSE INCLUDE_FILES src/main/*.h )
include_directories(${INCLUDE_FILES})
add_executable(${PROJECT_NAME} ${SRC_FILES} ${INCLUDE_FILES})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} libas02)

# tests

enable_testing()

add_test(NAME "part1-j2c-wrapping" COMMAND ${PROJECT_NAME} --in "${PROJECT_SOURCE_DIR}/src/test/resources/part1.j2c" --format J2C --out part1-j2c.mxf)

add_test(NAME "part15-j2c-wrapping" COMMAND ${PROJECT_NAME} --in "${PROJECT_SOURCE_DIR}/src/test/resources/part15.j2c" --format J2C --out part15-j2c.mxf)

add_test(NAME "part1-mjc-wrapping" COMMAND ${PROJECT_NAME} --in "${PROJECT_SOURCE_DIR}/src/test/resources/part1.mjc" --format MJC --out part1-mjc.mxf)

add_test(NAME "part15-mjc-wrapping" COMMAND ${PROJECT_NAME} --in "${PROJECT_SOURCE_DIR}/src/test/resources/part15.mjc" --format MJC --out part15-mjc.mxf)


# compiler settings

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
set_property(TARGET ${PROJECT_NAME} PROPERTY LINKER_LANGUAGE CXX)

# force warnings to be treated as errors

if ( CMAKE_COMPILER_IS_GNUCC )
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")
endif ( CMAKE_COMPILER_IS_GNUCC )

if ( MSVC )
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /W4")
endif ( MSVC )