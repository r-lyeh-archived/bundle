
project(libzling)
cmake_minimum_required(VERSION 2.8)

# source path
aux_source_directory("../src" DIR_SRC)
aux_source_directory("../demo" DIR_DEMO)

file(COPY "../src/libzling.h"       DESTINATION "./include/libzling")
file(COPY "../src/libzling_utils.h" DESTINATION "./include/libzling")
file(COPY "../src/libzling_inc.h"   DESTINATION "./include/libzling")
file(COPY "../src/msinttypes"       DESTINATION "./include/libzling")

include_directories("./include")

add_library(zling SHARED  ${DIR_SRC})
add_executable(zling_demo ${DIR_DEMO})

target_link_libraries(zling_demo zling)

# install
install(FILES     "../src/libzling.h"       DESTINATION "./include/libzling")
install(FILES     "../src/libzling_utils.h" DESTINATION "./include/libzling")
install(FILES     "../src/libzling_inc.h"   DESTINATION "./include/libzling")
install(DIRECTORY "../src/msinttypes"       DESTINATION "./include/libzling")
install(TARGETS zling                       DESTINATION "./lib")
install(TARGETS zling_demo                  DESTINATION "./bin")
