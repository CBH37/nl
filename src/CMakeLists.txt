PROJECT(nl)
CMAKE_MINIMUM_REQUIRED(VERSION 3.16)

FILE(GLOB SRC *.cpp)
FILE(GLOB HDR *.hpp)
ADD_EXECUTABLE(nl ${SRC} ${HDR})

# 为了实现外部函数需要做的一些跨平台设置
IF(CMAKE_SYSTEM_NAME MATCHES "Linux")
    TARGET_LINK_LIBRARIES(nl ${CMAKE_DL_LIBS}) # 链接`dlfcn.h`
ELSEIF(CMAKE_SYSTEM_NAME MATCHES "Windows")
ELSE()
    MESSAGE(FATAL_ERROR "the current platform is not supported")
ENDIF()

ADD_SUBDIRECTORY(std)   # 编译`nl`标准库