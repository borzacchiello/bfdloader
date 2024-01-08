cmake_minimum_required(VERSION 3.16)

project(bfdloader C)

include_directories(${CMAKE_SOURCE_DIR}/binutils/src/bfd)
include_directories(${CMAKE_SOURCE_DIR}/binutils/src/include)
link_directories(${CMAKE_SOURCE_DIR}/binutils/build)
link_directories(${CMAKE_BINARY_DIR})

# Build libbfd
if(NOT ${CMAKE_SOURCE_DIR}/binutils/build/libbfd.a)
    execute_process(COMMAND ${CMAKE_SOURCE_DIR}/binutils/build.sh)
endif()

# Create static library (by joining the new objects and the dependencies)
set(SOURCES bfdloader.c)

add_library("${PROJECT_NAME}-static" STATIC ${SOURCES})
add_custom_command(OUTPUT lib${PROJECT_NAME}.a
                   COMMAND rm ARGS -f *.o
                   COMMAND ar ARGS -x ${CMAKE_BINARY_DIR}/lib${PROJECT_NAME}-static.a
                   COMMAND ar ARGS -x ${CMAKE_SOURCE_DIR}/binutils/build/libbfd.a
                   COMMAND ar ARGS -x ${CMAKE_SOURCE_DIR}/binutils/build/libz.a
                   COMMAND ar ARGS -x ${CMAKE_SOURCE_DIR}/binutils/build/libsframe.a
                   COMMAND ar ARGS -x ${CMAKE_SOURCE_DIR}/binutils/build/libiberty.a
                   COMMAND ar ARGS -r lib${PROJECT_NAME}.a *.o
                   COMMAND rm ARGS -f *.o
                   DEPENDS "${PROJECT_NAME}-static")
add_custom_target(${PROJECT_NAME} ALL DEPENDS lib${PROJECT_NAME}.a)

add_executable(bfdinfo bfdinfo.c)
add_dependencies(bfdinfo ${PROJECT_NAME})
target_link_libraries(bfdinfo LINK_PUBLIC lib${PROJECT_NAME}.a)

install(FILES bfdloader.h DESTINATION include)
install(PROGRAMS ${CMAKE_BINARY_DIR}/lib${PROJECT_NAME}.a DESTINATION lib)
