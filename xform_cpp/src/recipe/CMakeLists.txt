find_package(Armadillo REQUIRED)
include_directories(${ARMADILLO_INCLUDE_DIRS})

#######################################################################
#                           Halide sources                            #
#######################################################################

set(halide_recipe_aot
      hl_lowpass
      hl_highpass
      hl_compute_features
      hl_precompute_pyramid
      hl_reconstruct
      hl_gaussian_downsample
      hl_laplacian_pyramid_level
      hl_laplacian_pyramid_level_with_noise
      hl_collapse_pyramid
      hl_upsample
)

set(halide_objs)
set(objdir ${obj}/recipe)
foreach(f ${halide_recipe_aot})
    halide_aot_compile(${f}
        BIN ${bin}/recipe
        OBJ ${objdir}
        INC ${inc}/recipe
    )
    list(APPEND halide_objs ${objdir}/${f}.o)
endforeach(f)


#######################################################################
#                      Transform Recipes Library                      #
#######################################################################

set(recipe_src
      Recipe.cpp
      ${halide_objs}
      client_preprocessing.cpp
      server_preprocessing.cpp
)

add_library(recipe STATIC ${recipe_src} ${gpu_fit_recipe})
target_link_libraries(recipe
    ${ARMADILLO_LIBRARIES}
    utils
    histogram
)

# Use OpenMP on g++
IF(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    FIND_PACKAGE(OpenMP)
    IF(OPENMP_FOUND)
        SET(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        MESSAGE("Using OpenMP")
    ELSE(OPENMP_FOUND)
        MESSAGE("Not using OpenMP: library not found")
    ENDIF(OPENMP_FOUND)
ELSE(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    MESSAGE("Not using OpenMP because compiler is not GCC/G++")
ENDIF(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)

# required on Ubuntu
IF(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    SET_TARGET_PROPERTIES(recipe PROPERTIES COMPILE_FLAGS "-fPIC")
ENDIF(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
