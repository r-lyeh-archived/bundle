# Written in 2009 by Lloyd Hilaiel
#
# License
# 
# All the cruft you find here is public domain.  You don't have to credit
# anyone to use this code, but my personal request is that you mention
# Igor Pavlov for his hard, high quality work.
#

SET (SRCS easylzma_test.c simple.c)
SET (HDRS simple.h)

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_BINARY_DIR}/../${EASYLZMA_DIST_NAME}/include
)
LINK_DIRECTORIES(
    ${CMAKE_CURRENT_BINARY_DIR}/../${EASYLZMA_DIST_NAME}/lib
)

ADD_EXECUTABLE(easylzma_test ${SRCS} ${HDRS})

TARGET_LINK_LIBRARIES(easylzma_test easylzma_s)

GET_TARGET_PROPERTY(binPath easylzma_test LOCATION)

# a test target
ADD_CUSTOM_TARGET(test ${binPath})
