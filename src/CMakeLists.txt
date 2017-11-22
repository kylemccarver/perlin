SET(pwd ${CMAKE_CURRENT_LIST_DIR})

SET(src "")
AUX_SOURCE_DIRECTORY(${pwd} src)
add_executable(perlin ${src})
message(STATUS "perlin added ${src}")

target_link_libraries(perlin ${stdgl_libraries})
FIND_PACKAGE(JPEG REQUIRED)
TARGET_LINK_LIBRARIES(perlin ${JPEG_LIBRARIES})
