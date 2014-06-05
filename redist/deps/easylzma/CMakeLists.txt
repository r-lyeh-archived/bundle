# Written in 2009 by Lloyd Hilaiel
#
# License
# 
# All the cruft you find here is public domain.  You don't have to credit
# anyone to use this code, but my personal request is that you mention
# Igor Pavlov for his hard, high quality work.
#

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(easylzma)

SET (EASYLZMA_MAJOR 0)
SET (EASYLZMA_MINOR 0)
SET (EASYLZMA_MICRO 8)

SET (EASYLZMA_DIST_NAME
     "easylzma-${EASYLZMA_MAJOR}.${EASYLZMA_MINOR}.${EASYLZMA_MICRO}")

IF (NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "Release")
ENDIF (NOT CMAKE_BUILD_TYPE)

IF (${CMAKE_BUILD_TYPE} STREQUAL "Release")
  MESSAGE("** for a debug build: cmake -DCMAKE_BUILD_TYPE=Debug ..")
ENDIF (${CMAKE_BUILD_TYPE} STREQUAL "Release")

SET(CMAKE_C_FLAGS "-Wall")
IF (WIN32) 
  SET(linkFlags "/PDB:NONE /INCREMENTAL:NO /OPT:NOREF /OPT:NOICF")
  SET(CMAKE_EXE_LINKER_FLAGS    "${linkFlags}"
      CACHE STRING "LZMA linker flags" FORCE)
  SET(CMAKE_EXE_LINKER_FLAGS_DEBUG ""
      CACHE STRING "LZMA debug linker flags" FORCE)
  SET(CMAKE_EXE_LINKER_FLAGS_RELEASE
      CACHE STRING "LZMA release linker flags" FORCE)
  SET(CMAKE_SHARED_LINKER_FLAGS "${linkFlags}"
      CACHE STRING "LZMA shared linker flags" FORCE)
  SET(CMAKE_MODULE_LINKER_FLAGS "${linkFlags}"
      CACHE STRING "LZMA module linker flags" FORCE)

  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4668 /wd4820 /wd4996 /wd4255 /wd4130 /wd4100 /wd4711")
  SET(CMAKE_C_FLAGS_DEBUG "/D DEBUG /Od /Z7")
  SET(CMAKE_C_FLAGS_RELEASE "/D NDEBUG /O2")
ELSE (WIN32)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -ansi")
  SET(CMAKE_C_FLAGS_DEBUG "-DDEBUG -g")
  SET(CMAKE_C_FLAGS_RELEASE "-DNDEBUG -O2")
ENDIF (WIN32) 

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(elzma)
ADD_SUBDIRECTORY(test)
