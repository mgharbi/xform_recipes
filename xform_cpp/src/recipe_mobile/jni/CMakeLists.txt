find_package(JNI REQUIRED)
include_directories(${JNI_INCLUDE_DIRS})

#######################################################################
#                           Halide sources                            #
#######################################################################


set(halide_objs)
halide_arm_compile(hl_local_laplacian
    BIN ${bin}/filters/local_laplacian
    OBJ ${obj}/${ANDROID_ABI}
    INC ${inc}/${ANDROID_ABI}
)
list(APPEND halide_objs ${obj}/${ANDROID_ABI}/hl_local_laplacian.o)

set(halide_style_transfer_files
    hl_rgb2gray
    hl_gray2rgb
    hl_style_transfer
)
foreach(f ${halide_style_transfer_files})
    halide_arm_compile(${f}
        BIN ${bin}/filters/style_transfer
        OBJ ${obj}/${ANDROID_ABI}
        INC ${inc}/${ANDROID_ABI}
    )
list(APPEND halide_objs ${obj}/${ANDROID_ABI}/${f}.o)
endforeach(f)

set(halide_histogram_files
    hl_histogram_of_gradients
    hl_histogram
    hl_histogram_color
    hl_histogram_color_float
    hl_transfer_histogram
    hl_transfer_histogram_color
    hl_transfer_histogram_color_float
    hl_histogram_of_laplacian_coefficients
)
foreach(f ${halide_histogram_files})
    halide_arm_compile(${f}
        BIN ${bin}/utils/histogram
        OBJ ${obj}/${ANDROID_ABI}
        INC ${inc}/${ANDROID_ABI}
    )
list(APPEND halide_objs ${obj}/${ANDROID_ABI}/${f}.o)
endforeach(f)

set(halide_colorization_files
    hl_nonzero
    hl_fuse_yuv
    hl_yuv2rgb
)
foreach(f ${halide_colorization_files})
    halide_arm_compile(${f}
        BIN ${bin}/filters/colorization
        OBJ ${obj}/${ANDROID_ABI}
        INC ${inc}/${ANDROID_ABI}
    )
list(APPEND halide_objs ${obj}/${ANDROID_ABI}/${f}.o)
endforeach(f)

# Recipe Halide
set(halide_recipe_files
    hl_lowpass
    hl_highpass
    hl_reconstruct
    hl_gaussian_downsample
    hl_precompute_pyramid
)

foreach(f ${halide_recipe_files})
    halide_arm_compile(${f}
        BIN ${bin}/recipe
        OBJ ${obj}/${ANDROID_ABI}
        INC ${inc}/${ANDROID_ABI}
    )
    list(APPEND halide_objs ${obj}/${ANDROID_ABI}/${f}.o)
endforeach(f)

#######################################################################
#                         Java Native Library                         #
#######################################################################


set(additional_sources
    ${src}/utils/histogram/histogram.cpp
    ${src}/utils/histogram/histogram_io.cpp
    ${src}/recipe/Recipe.cpp
    ${src}/recipe/client_preprocessing.cpp
    ${src}/filters/style_transfer/style_transfer.cpp
    ${src}/filters/colorization/colorize.cpp
    ${src}/filters/colorization/fmg.cpp
    ${src}/filters/colorization/mg.cpp
    ${src}/filters/colorization/tensor2d.cpp
    ${src}/filters/colorization/tensor3d.cpp
)

set(ANDROID_SRC 
    recipe_mobile.cpp
    RecipeState.cpp
    input_preprocessing.cpp
    precomputation.cpp
    image_utils.cpp
)
set(ANDROID_MODULE RecipeMobileJNI)

# Generate Android native lib
add_custom_command(
    OUTPUT ${lib}/${ANDROID_ABI}/lib${ANDROID_MODULE}.so
    DEPENDS ${halide_objs} ${ANDROID_SRC} ${additional_sources}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}

    COMMAND
    NDK_OUT=${obj}/${ANDROID_MODULE}
    ANDROID_LIB_DIR=${lib}
    ANDROID_SRC="${ANDROID_SRC};${additional_sources}"
    ANDROID_MODULE=${ANDROID_MODULE}
    ANDROID_C_INCLUDES=${inc}
    HALIDE_INCLUDES=${HALIDE_INCLUDES}
    ANDROID_LD_LIBS="${halide_objs}"
    ANDROID_ABI=${ANDROID_ABI}
    ndk-build
)

add_custom_target(android_jni ALL 
    DEPENDS ${lib}/${ANDROID_ABI}/lib${ANDROID_MODULE}.so
)
