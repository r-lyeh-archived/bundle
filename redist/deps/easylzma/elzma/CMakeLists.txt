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

# set up some paths
SET (binDir ${CMAKE_CURRENT_BINARY_DIR}/../${EASYLZMA_DIST_NAME}/bin)

# create some directories
FILE(MAKE_DIRECTORY ${binDir})

SET (SRCS elzma.c util.c)
SET (HDRS util.h)

# use the library we built
INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_BINARY_DIR}/../${EASYLZMA_DIST_NAME}/include
)
LINK_DIRECTORIES(
    ${CMAKE_CURRENT_BINARY_DIR}/../${EASYLZMA_DIST_NAME}/lib
)

ADD_EXECUTABLE(elzma ${SRCS})

TARGET_LINK_LIBRARIES(elzma easylzma_s)

# make a hard link (or copy on win32) from unelzma to elzma
GET_TARGET_PROPERTY(binPath elzma LOCATION)
IF (WIN32)
  SET(exeExt ".exe") 
  SET(hardLinkCommand ${CMAKE_COMMAND} -E copy_if_different) 
ELSE (WIN32)
  SET(hardLinkCommand ln -f) 
  SET(exeExt ) 
ENDIF (WIN32)
ADD_CUSTOM_COMMAND(TARGET elzma POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${binPath} ${binDir}
      COMMAND ${hardLinkCommand} elzma${exeExt} unelzma${exeExt}
      WORKING_DIRECTORY "${binDir}")    
