CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(geolocation C)

ADD_DEFINITIONS(-g -O0 -DDEBUG -D_XOPEN_SOURCE -D_BSD_SOURCE)

ADD_LIBRARY(geolocation STATIC geolocation.c radix_tree.c avl.c)

ADD_EXECUTABLE(main main.c)
TARGET_LINK_LIBRARIES(main geolocation m)
