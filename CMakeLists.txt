

cmake_minimum_required(VERSION 3.8)
set(CMAKE_CXX_STANDARD 14)
project(NetWorkSever)
# cmake modules
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

###############################################################################
# Libraries
###############################################################################
#find_package(Gflags REQUIRED)                                   # gflags
#find_package(Glog REQUIRED)                                     # glog
#find_package(Boost COMPONENTS filesystem date_time REQUIRED)    # boost
find_package(Boost  REQUIRED COMPONENTS system thread)           # boost asio
#find_package(Eigen3 REQUIRED)                                   # Eigen
find_package(OpenCV 3 REQUIRED)                                 # OpenCV
## public and prive dependency include directories
list(APPEND PUBLIC_DEPEND_INCLUDES
#        ${GFLAGS_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIRS}
        )
list(APPEND PRIVATE_DEPEND_INCLUDES
#        ${GLOG_INCLUDE_DIRS}
#        ${EIGEN3_INCLUDE_DIRS}
        ${OpenCV_INCLUDE_DIRS}
        )
## public and private dependency libraries
#list(APPEND PUBLIC_DEPEND_LIBS
#        ${GFLAGS_LIBRARIES}
#        )
list(APPEND PRIVATE_DEPEND_LIBS
#        ${GLOG_LIBRARIES}
        ${Boost_LIBRARIES}
        ${OpenCV_LIBRARIES}
#        ueye_api turbojpeg
        )

include_directories(
#        ${GFLAGS_INCLUDE_DIRS}
#        ${GLOG_INCLUDE_DIRS}
#        ${EIGEN3_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIRS}
        ${OpenCV_INCLUDE_DIRS}
)
set(EXTERNAL_LIBS ${EXTERNAL_LIBS}
#        ${GFLAGS_LIBRARIES}
#        ${GLOG_LIBRARIES}
        ${Boost_LIBRARIES}
        ${OpenCV_LIBRARIES}
        )

###############################################################################
# Project Setting
###############################################################################
#default built type
IF (NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE RELEASE)
ENDIF ()
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
# build setting
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
option(NO_VERIFICATION "Don`t send verification information to server for SA" NO)
###############################################################################
# Application
###############################################################################
# feaext lib
set(PRJ_LIB fexext)
add_subdirectory(UDP)
add_subdirectory(TCP)
add_subdirectory(UDP-Img)
add_subdirectory(UDP-LargeFile)
add_subdirectory(UDP-BigFileRec)
add_subdirectory(UDPSer-GetServerIP)
add_subdirectory(UDPNewHeaderServer)
add_subdirectory(UDPServerBoost)
