cmake_minimum_required(VERSION 2.8)
project(pimp)
include_directories(include)

file(GLOB PIMP_HEADERS include/*.h)
install(FILES ${PIMP_HEADERS} DESTINATION include)
file(GLOB PIMP_SOURCES src/*.c)
set(PIMP_SOURCES ${PIMP_SOURCES} ${PIMP_HEADERS})
add_library(pimp SHARED ${PIMP_SOURCES})
install(TARGETS pimp DESTINATION lib)

add_executable(pimp-echo examples/echo/main.c)
target_link_libraries(pimp-echo
    pimp)
install(TARGETS pimp-echo DESTINATION bin)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
