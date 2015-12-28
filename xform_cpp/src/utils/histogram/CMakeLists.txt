set(halide_histogram_aot
    hl_histogram_of_gradients
    hl_histogram
    hl_histogram_color
    hl_histogram_color_float
    hl_transfer_histogram
    hl_transfer_histogram_color
    hl_transfer_histogram_color_float
    hl_histogram_of_laplacian_coefficients
)

set(halide_objs)
set(objdir ${obj}/utils/histogram)
foreach(f ${halide_histogram_aot})
    halide_aot_compile(${f}
        BIN ${bin}/utils/histogram
        OBJ ${objdir}
        INC ${inc}/utils/histogram
    )
    list(APPEND halide_objs ${objdir}/${f}.o)
endforeach(f)

add_library(histogram STATIC
    histogram_io.cpp
    histogram.cpp
    ${halide_objs}
)

SET_TARGET_PROPERTIES (
    histogram
    PROPERTIES LINKER_LANGUAGE CXX
)

# required on Ubuntu
IF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
    SET_TARGET_PROPERTIES(histogram PROPERTIES COMPILE_FLAGS "-fPIC")
ENDIF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
