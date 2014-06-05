# Written in 2009 by Lloyd Hilaiel
#
# License
# 
# All the cruft you find here is public domain.  You don't have to credit
# anyone to use this code, but my personal request is that you mention
# Igor Pavlov for his hard, high quality work.
#

IF (WIN32)
  ADD_DEFINITIONS(-DWIN32)
ENDIF (WIN32)

FILE(GLOB SRCS pavlov/*.c *.c)
FILE(GLOB HDRS *.h pavlov/*.h easylzma/*.h)
FILE(GLOB PUB_HDRS easylzma/*.h)

# set up some paths for outputing the usable binaries
SET (libDir
     ${CMAKE_CURRENT_BINARY_DIR}/../${EASYLZMA_DIST_NAME}/lib)
SET (incDir
     ${CMAKE_CURRENT_BINARY_DIR}/../${EASYLZMA_DIST_NAME}/include/easylzma)

# an include directory to allow easylzma implementation to find public
# headers
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

# get the built libs into the correct place
SET(LIBRARY_OUTPUT_PATH ${libDir})

ADD_LIBRARY(easylzma_s STATIC ${SRCS} ${HDRS})
ADD_LIBRARY(easylzma   SHARED ${SRCS} ${HDRS})

# setup shared library version numbering
SET_TARGET_PROPERTIES(
    easylzma PROPERTIES
    SOVERSION ${EASYLZMA_MAJOR}
    VERSION ${EASYLZMA_MAJOR}.${EASYLZMA_MINOR}.${EASYLZMA_MICRO})

# on win32 we'll need to setup exports correctly
SET(sharedLibCompileFlags "-DEASYLZMA_SHARED -DEASYLZMA_BUILD")
IF (APPLE)
  SET(sharedLibCompileFlags "${sharedLibCompileFlags} -fno-common")
ENDIF (APPLE)

SET_TARGET_PROPERTIES(easylzma PROPERTIES
                      COMPILE_FLAGS ${sharedLibCompileFlags})

# create these output directories
FILE(MAKE_DIRECTORY ${libDir})
FILE(MAKE_DIRECTORY ${incDir})

### copy the two required headers into our output dir as a post build step
# copy public headers to output directory
FOREACH (header ${PUB_HDRS})
  # preserve relative pathing		
  ADD_CUSTOM_COMMAND(TARGET easylzma_s POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${header} ${incDir})
ENDFOREACH (header ${PUB_HDRS})
