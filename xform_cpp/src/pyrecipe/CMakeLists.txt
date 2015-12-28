# Swig for python c++ wrapper
find_package(SWIG REQUIRED)
find_package(PythonInterp)
find_package(PythonLibs)
find_package(NumPy)

message(interp: ${PYTHON_EXECUTABLE})
message(PY ${PYTHON_INCLUDE_DIRS})
message(numPY ${NUMPY_INCLUDE_DIRS})

include_directories(${PYTHON_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${NUMPY_INCLUDE_DIRS})

include(${SWIG_USE_FILE})
set(CMAKE_SWIG_FLAGS "-c++")

set_source_files_properties(numpy.i PROPERTIES CPLUSPLUS ON)
set_source_files_properties(pyrecipe.i PROPERTIES CPLUSPLUS ON)
set_source_files_properties(pyrecipe.i PROPERTIES SWIG_FLAGS "-includeall")

# Use OpenMP on g++
IF(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    FIND_PACKAGE(OpenMP)
    IF(OPENMP_FOUND)
        SET(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        MESSAGE("Using OpenMP")
        SET_SOURCE_FILES_PROPERTIES(${swig_generated_file_fullname} PROPERTIES COMPILE_FLAGS "${OpenMP_C_FLAGS} -gomp")
    ELSE(OPENMP_FOUND)
        MESSAGE("Not using OpenMP: library not found")
    ENDIF(OPENMP_FOUND)
ELSE(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    MESSAGE("Not using OpenMP because compiler is not GCC/G++")
ENDIF(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)

swig_add_module(pyrecipe python
    pyrecipe.i
    pyrecipe.cpp
)


swig_link_libraries(pyrecipe
    ${PYTHON_LIBRARIES}
    local_laplacian
    style_transfer
    colorization
    recipe
    utils
)

add_custom_command(
    TARGET ${SWIG_MODULE_pyrecipe_REAL_NAME}
    POST_BUILD
    COMMAND
    ${CMAKE_COMMAND} ARGS -E copy ${CMAKE_CURRENT_BINARY_DIR}/pyrecipe.py ${pywrapper}
)
