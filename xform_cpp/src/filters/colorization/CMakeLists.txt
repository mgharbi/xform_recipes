set(colorization_src
    colorize.cpp
    fmg.cpp
    mg.cpp
    tensor2d.cpp
    tensor3d.cpp
)

set(colorization_aot
      hl_fuse_yuv
      hl_nonzero
      hl_yuv2rgb
)

set(halide_objs)
set(objdir ${obj}/filters/colorization)
foreach(f ${colorization_aot})
    halide_aot_compile(${f}
        BIN ${bin}/filters/colorization
        OBJ ${objdir}
        INC ${inc}/filters/colorization
    )
    list(APPEND halide_objs ${objdir}/${f}.o)
endforeach(f)

add_library(colorization STATIC
    ${colorization_src}
    ${halide_objs}
)

target_link_libraries(colorization
    ${HALIDE_LIBRARY}
)
