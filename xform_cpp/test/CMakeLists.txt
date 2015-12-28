find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
set(test_bindir ${bin}/test)

add_executable(test_histogram setup.cpp test_histogram.cpp)
add_executable(test_filters setup.cpp test_filters.cpp)
add_executable(test_io setup.cpp test_io.cpp)
add_executable(test_recipe setup.cpp test_recipe.cpp)

GTEST_ADD_TESTS(test_histogram "" test_histogram.cpp)
GTEST_ADD_TESTS(test_filters "" test_filters.cpp)
GTEST_ADD_TESTS(test_io "" test_io.cpp)
GTEST_ADD_TESTS(test_recipe "" test_recipe.cpp)

set_target_properties(test_histogram PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${test_bindir})
set_target_properties(test_filters PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${test_bindir})
set_target_properties(test_io PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${test_bindir})
set_target_properties(test_recipe PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${test_bindir})

target_link_libraries(test_histogram
    ${GTEST_BOTH_LIBRARIES}
    ${Boost_LIBRARIES}
    histogram
    pthread
)

target_link_libraries(test_filters
    ${GTEST_BOTH_LIBRARIES}
    ${Boost_LIBRARIES}
    local_laplacian
    colorization
    style_transfer
    utils
    pthread
)

target_link_libraries(test_io
    ${GTEST_BOTH_LIBRARIES}
    ${Boost_LIBRARIES}
    utils
)

target_link_libraries(test_recipe
    ${GTEST_BOTH_LIBRARIES}
    ${Boost_LIBRARIES}
    pthread
    recipe
    local_laplacian
)
