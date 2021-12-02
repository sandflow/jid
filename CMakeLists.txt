cmake_minimum_required (VERSION 2.8)
project(jid)

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

# coomon includes

include_directories(src/main)

# jid-writer

set(JID_WRITER "jid-writer")
add_executable(${JID_WRITER} src/main/jid-writer.cpp src/main/CodestreamSequence.cpp)
target_link_libraries(${JID_WRITER} ${Boost_LIBRARIES} libas02)

# jid-reader

set(JID_READER "jid-reader")
add_executable(${JID_READER} src/main/jid-reader.cpp)
target_link_libraries(${JID_READER} ${Boost_LIBRARIES} libas02)

# tests

enable_testing()

add_test(NAME "part1-j2c-wrapping" COMMAND ${JID_WRITER} --in "${PROJECT_SOURCE_DIR}/src/test/resources/part1.j2c" --format J2C --out part1-j2c.mxf)

add_test(NAME "part15-j2c-wrapping" COMMAND ${JID_WRITER} --in "${PROJECT_SOURCE_DIR}/src/test/resources/part15.j2c" --format J2C --out part15-j2c.mxf)

add_test(NAME "part1-mjc-wrapping" COMMAND ${JID_WRITER} --in "${PROJECT_SOURCE_DIR}/src/test/resources/part1.mjc" --format MJC --out part1-mjc.mxf)

add_test(NAME "mjc-cbr-wrapping" COMMAND ${JID_WRITER} --in "${PROJECT_SOURCE_DIR}/src/test/resources/crowdrun-lowlatency.1920x1080-422-10bit-50p.mjc" --color COLOR.3 --quantization QE.1 --components YCbCr --format MJC --out part15-mjc-cbr.mxf)

add_test(NAME "part15-mjc-wrapping" COMMAND ${JID_WRITER} --in "${PROJECT_SOURCE_DIR}/src/test/resources/part15.mjc" --format MJC --out part15-mjc.mxf)

add_test(NAME "yuv422_10b_p15-wrapping" COMMAND ${JID_WRITER} --color COLOR.3 --quantization QE.1 --components YCbCr --format J2C  --in "${PROJECT_SOURCE_DIR}/src/test/resources/yuv422_10b_p15.j2c" --out yuv422_10b_p15.mxf)

add_test(NAME "j2c-seq-wrapping" COMMAND ${JID_WRITER} --color COLOR.3 --quantization QE.1 --components YCbCr --format J2C  --in "${PROJECT_SOURCE_DIR}/src/test/resources/j2c-sequence" --out j2c-seq.mxf)

add_test(NAME "j2c-wrapping-with-areas" COMMAND ${JID_WRITER}
	--in "${PROJECT_SOURCE_DIR}/src/test/resources/part1.j2c"
	--out j2c-wrapping-with-areas.mxf
	--format J2C
	--display_area 10 20 1000 1010
	--active_area 10 20 500 510
	)

add_test(NAME "j2c-wrapping-with-color-volume" COMMAND ${JID_WRITER}
	--in "${PROJECT_SOURCE_DIR}/src/test/resources/part1.j2c"
	--out j2c-wrapping-with-color-volume.mxf
	--format J2C
	--color COLOR.3
	--mastering_display_primaries 35400 14600 8500 39850 6550 2300
	--mastering_display_white_point_chroma 15635 16450
	--mastering_display_max_luminance 10000000
	--mastering_display_min_luminance 50
	)

set(J2C_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/j2c-out)

file(MAKE_DIRECTORY ${J2C_OUT_DIR})

add_test(NAME "unwrapping-j2c" COMMAND ${JID_READER} --in part1-mjc.mxf --format J2C --out ${J2C_OUT_DIR})

add_test(NAME "unwrapping-mjc-file" COMMAND ${JID_READER} --in part1-mjc.mxf --format MJC --out "out.mjc")

add_test(NAME "unwrapping-mjc-stdout" COMMAND ${JID_READER} --in part1-mjc.mxf --format MJC)

# compiler settings

set_property(DIRECTORY PROPERTY CXX_STANDARD 11)
set_property(DIRECTORY PROPERTY LINKER_LANGUAGE CXX)

# force warnings to be treated as errors

if ( CMAKE_COMPILER_IS_GNUCC )
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")
endif ( CMAKE_COMPILER_IS_GNUCC )

if ( MSVC )
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /W4")
endif ( MSVC )