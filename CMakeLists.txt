PROJECT(FBIMiner CXX)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

FIND_PACKAGE( Boost 1.40 COMPONENTS program_options REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )


SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall -O2 -Wno-sign-compare -Wextra")
FILE(GLOB_RECURSE DataSrcs data/*.cpp)
FILE(GLOB_RECURSE AlgSrcs algorithms/*.cpp)
FILE(GLOB_RECURSE UtilSrcs util/*.cpp)
ADD_EXECUTABLE(CTane main.cpp ${DataSrcs} ${AlgSrcs} ${UtilSrcs})
TARGET_LINK_LIBRARIES( CTane ${Boost_LIBRARIES} )
