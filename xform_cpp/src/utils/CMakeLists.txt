find_package(ZLIB REQUIRED)

find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIRS})

find_package(JPEG REQUIRED)
include_directories(${JPEG_INCLUDE_DIRS})

set(utils_src
    image_io.cpp
    recipe_io.cpp
)

add_library(utils STATIC
    ${utils_src}
)

target_link_libraries(utils
    ${ZLIB_LIBRARIES}
    ${PNG_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${HALIDE_LIBRARY}
)

# required on Ubuntu
IF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
    SET_TARGET_PROPERTIES(utils PROPERTIES COMPILE_FLAGS "-fPIC")
ENDIF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )

add_subdirectory(histogram)
