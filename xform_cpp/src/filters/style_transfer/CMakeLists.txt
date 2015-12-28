set(halide_style_transfer_aot
    hl_rgb2gray
    hl_gray2rgb
    hl_style_transfer
)

set(halide_objs)
set(objdir ${obj}/filters/style_transfer)
foreach(f ${halide_style_transfer_aot})
    halide_aot_compile(${f}
        BIN ${bin}/filters/style_transfer
        OBJ ${objdir}
        INC ${inc}/filters/style_transfer
    )
    list(APPEND halide_objs ${objdir}/${f}.o)
endforeach(f)

add_library(style_transfer STATIC
    style_transfer.cpp
    ${halide_objs}
)

SET_TARGET_PROPERTIES (
    style_transfer
    PROPERTIES LINKER_LANGUAGE CXX
)

target_link_libraries(style_transfer
    histogram
)
