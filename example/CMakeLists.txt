# example/CMakeLists.txt
cmake_minimum_required(VERSION 2.6)
PROJECT(AVGL_EXAMPLES)

SET(CMAKE_VERBOSE_MAKEFILE OFF)

ADD_DEFINITIONS(
-W
-Wall
-Wcast-align
-Wmissing-prototypes
-Wstrict-prototypes
-Wnested-externs
-Wpointer-arith
-Wshadow
-Wwrite-strings
-Wcast-qual
-Waggregate-return
-Winline
-Wundef
-Wsign-compare
-Wredundant-decls
-g3
-ggdb3
-std=c99
)

SET(AVGL_SOURCE_DIR ${AVGL_EXAMPLES_SOURCE_DIR}/..)
SET(AVGL_LIB_DIR ${AVGL_SOURCE_DIR}/lib)

LINK_LIBRARIES(avgl)

OPTION(WITH_PTHREADS "Build with pthreads" YES)
OPTION(WITH_FFMPEG "Build with ffmpeg" NO)
OPTION(WITH_SDL "Build with SDL system backend" YES)
OPTION(WITH_DIRECTFB "Build with DirectFB system backend" NO)
OPTION(WITH_CAIRO "Build with Cairo graphics" YES)
OPTION(CAIRO_DIRECTFB "Cairo is build with DirectFB support" NO)

INCLUDE_DIRECTORIES(
	${AVGL_SOURCE_DIR}/include
	${AVGL_SOURCE_DIR}/build
	/usr/local/include/avgl
	/usr/include/avgl
)

LINK_DIRECTORIES(
	${AVGL_LIB_DIR}
)

# SET(EXE_DIR ${${PROJECT_NAME}_SOURCE_DIR}/build/bin)
# SET(EXECUTABLE_OUTPUT_PATH ${EXE_DIR})

SUBDIRS(core system video audio sound graphics window media player extensions)
